#!/usr/bin/env python3
"""Cross-platform C++ submission worker.

Uses a local subprocess instead of Docker so the evaluator can run on a normal
Windows/Linux development machine. It isolates each submission in a temporary
folder, applies a wall-clock timeout, caps captured output, and kills the
process tree when the limit is exceeded.
"""
from __future__ import annotations
import argparse, json, os, pathlib, sqlite3, subprocess, tempfile, time, threading, signal, shutil, ctypes
from datetime import datetime, timezone

MAX_SOURCE_BYTES = 32 * 1024
MAX_OUTPUT_BYTES = 64 * 1024
DEFAULT_TIMEOUT_MS = 2000
DEFAULT_MEMORY_MB = 128
REWARD_COINS = 10


def timestamp() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def connect(path: str) -> sqlite3.Connection:
    db = sqlite3.connect(path, timeout=5)
    db.row_factory = sqlite3.Row
    db.execute("PRAGMA foreign_keys=ON")
    db.execute("PRAGMA busy_timeout=5000")
    return db


def ensure_schema(db: sqlite3.Connection) -> None:
    db.executescript("""
      CREATE TABLE IF NOT EXISTS code_submission_rewards (
        user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,
        submission_id INTEGER NOT NULL REFERENCES code_submissions(id) ON DELETE CASCADE,
        coins_awarded INTEGER NOT NULL DEFAULT 10,
        awarded_at TEXT NOT NULL,
        PRIMARY KEY(user_id, exercise_id)
      );
      CREATE INDEX IF NOT EXISTS code_submissions_queue_idx
        ON code_submissions(status, id);
    """)
    db.execute("UPDATE code_submissions SET status='queued',message='Recovered after evaluator restart.' WHERE status='evaluating' AND finished_at=''" )
    db.commit()


def load_cases(case_dir: pathlib.Path, exercise_id: int) -> list[dict]:
    path = case_dir / f"{exercise_id}.json"
    if not path.is_file():
        raise ValueError(f"No test cases configured for exercise {exercise_id}")
    raw = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(raw, list) or not raw:
        raise ValueError("Test case file must be a non-empty JSON array")
    cases = []
    for index, case in enumerate(raw):
        if (not isinstance(case, dict) or
            not isinstance(case.get("input", ""), str) or
            not isinstance(case.get("expectedOutput"), str) or
            not isinstance(case.get("hidden", False), bool)):
            raise ValueError(f"Invalid test case at index {index}")
        timeout = int(case.get("timeLimitMs", DEFAULT_TIMEOUT_MS))
        memory = int(case.get("memoryLimitMb", DEFAULT_MEMORY_MB))
        if not 100 <= timeout <= 10000 or not 16 <= memory <= 512:
            raise ValueError(f"Invalid resource limits at index {index}")
        cases.append({"input": case.get("input", ""), "expectedOutput": case["expectedOutput"],
                      "hidden": case.get("hidden", False), "timeLimitMs": timeout,
                      "memoryLimitMb": memory})
    return cases


def normalize_output(value: str) -> str:
    return "\n".join(line.rstrip(" \t\r") for line in value.replace("\r\n", "\n").replace("\r", "\n").split("\n")).rstrip("\n")


def _kill_process(proc: subprocess.Popen) -> None:
    try:
        if os.name == "nt":
            proc.kill()
        else:
            os.killpg(proc.pid, signal.SIGKILL)
    except (ProcessLookupError, OSError):
        try:
            proc.kill()
        except OSError:
            pass


def _start_process(command: list[str], **kwargs) -> subprocess.Popen:
    if os.name == "nt":
        return subprocess.Popen(command, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP, **kwargs)
    return subprocess.Popen(command, start_new_session=True, **kwargs)


def _memory_usage_bytes(pid: int) -> int | None:
    """Return resident memory for a process where the host OS exposes it."""
    if os.name == "nt":
        try:
            class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
                _fields_ = [("cb", ctypes.c_ulong), ("PageFaultCount", ctypes.c_ulong),
                            ("PeakWorkingSetSize", ctypes.c_size_t), ("WorkingSetSize", ctypes.c_size_t),
                            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t), ("QuotaPagedPoolUsage", ctypes.c_size_t),
                            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t), ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                            ("PagefileUsage", ctypes.c_size_t), ("PeakPagefileUsage", ctypes.c_size_t)]
            counters = PROCESS_MEMORY_COUNTERS()
            counters.cb = ctypes.sizeof(counters)
            PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
            handle = ctypes.windll.kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
            if not handle:
                return None
            try:
                ok = ctypes.windll.psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb)
                return int(counters.WorkingSetSize) if ok else None
            finally:
                ctypes.windll.kernel32.CloseHandle(handle)
        except (AttributeError, OSError):
            return None
    try:
        return int(pathlib.Path(f"/proc/{pid}/status").read_text().split("VmRSS:", 1)[1].split()[0]) * 1024
    except (FileNotFoundError, IndexError, ValueError):
        return None


def _run_limited(command: list[str], input_data: bytes, timeout_s: float, memory_limit_mb: int) -> tuple[int, bytes, bool, bool, bool]:
    """Run command and return (returncode, output, timed_out, output_limited, memory_limited)."""
    proc = _start_process(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    chunks: list[bytes] = []
    total = 0
    limited = False
    memory_limited = False
    lock = threading.Lock()

    def reader() -> None:
        nonlocal total, limited
        while True:
            chunk = proc.stdout.read(4096)
            if not chunk:
                break
            with lock:
                remaining = MAX_OUTPUT_BYTES + 1 - total
                if remaining > 0:
                    chunks.append(chunk[:remaining])
                    total += min(len(chunk), remaining)
                if total > MAX_OUTPUT_BYTES:
                    limited = True
                    _kill_process(proc)
                    break

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    try:
        try:
            proc.stdin.write(input_data)
            proc.stdin.close()
        except (BrokenPipeError, OSError):
            pass
        deadline = time.monotonic() + timeout_s
        timed_out = False
        memory_limit_bytes = memory_limit_mb * 1024 * 1024
        while proc.poll() is None:
            usage = _memory_usage_bytes(proc.pid)
            if usage is not None and usage > memory_limit_bytes:
                memory_limited = True
                _kill_process(proc)
                break
            if time.monotonic() >= deadline:
                timed_out = True
                _kill_process(proc)
                break
            time.sleep(0.01)
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        timed_out = True
        _kill_process(proc)
        proc.wait()
    finally:
        thread.join(timeout=2)
        try:
            proc.stdout.close()
        except (AttributeError, OSError):
            pass
    return proc.returncode, b"".join(chunks), timed_out, limited, memory_limited


def run_local(source: str, case: dict, compiler: str = "g++") -> tuple[str, str]:
    """Compile and execute C++ locally in a temporary directory."""
    compiler_path = shutil.which(compiler) or compiler
    if not shutil.which(compiler) and not pathlib.Path(compiler).exists():
        return "internal_error", f"C++ compiler not found: {compiler}"
    with tempfile.TemporaryDirectory(prefix="pet-judge-") as temp:
        root = pathlib.Path(temp)
        src = root / "main.cpp"
        exe = root / ("main.exe" if os.name == "nt" else "main")
        src.write_text(source, encoding="utf-8")
        compile_cmd = [compiler_path, "-std=c++17", "-O2", str(src), "-o", str(exe)]
        try:
            cp = subprocess.run(compile_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                timeout=15, cwd=temp, check=False)
        except subprocess.TimeoutExpired:
            return "compile_error", "Compilation exceeded 15 seconds."
        compile_output = cp.stdout[:MAX_OUTPUT_BYTES].decode("utf-8", errors="replace")
        if cp.returncode != 0:
            return "compile_error", compile_output[-4000:] or "Compilation failed."

        timeout_s = case["timeLimitMs"] / 1000.0
        rc, output, timed_out, limited, memory_limited = _run_limited([str(exe)], case["input"].encode("utf-8"), timeout_s, case["memoryLimitMb"])
        text = output.decode("utf-8", errors="replace")
        if timed_out:
            return "time_limit_exceeded", "Program exceeded the time limit."
        if limited:
            return "runtime_error", "Output exceeded 64 KiB limit."
        if memory_limited:
            return "memory_limit_exceeded", f"Program exceeded the {case['memoryLimitMb']} MiB memory limit."
        if rc != 0:
            return "runtime_error", f"Program exited with code {rc}. {text[-1000:]}".strip()
        return "ok", text


def claim_one(db: sqlite3.Connection):
    db.execute("BEGIN IMMEDIATE")
    row = db.execute("SELECT id,user_id,exercise_id,source_code,language FROM code_submissions WHERE status='queued' ORDER BY id LIMIT 1").fetchone()
    if row is None:
        db.commit()
        return None
    changed = db.execute("UPDATE code_submissions SET status='evaluating', message='Evaluation started.' WHERE id=? AND status='queued'", (row['id'],)).rowcount
    db.commit()
    return dict(row) if changed else None


def finish(db: sqlite3.Connection, sub: dict, status: str, score: int, message: str) -> None:
    now = timestamp()
    db.execute("BEGIN IMMEDIATE")
    db.execute("UPDATE code_submissions SET status=?,score=?,message=?,finished_at=? WHERE id=?",
               (status, score, message[:4000], now, sub['id']))
    if status == "accepted":
        cur = db.execute("INSERT OR IGNORE INTO code_submission_rewards(user_id,exercise_id,submission_id,coins_awarded,awarded_at) VALUES(?,?,?,?,?)",
                         (sub['user_id'], sub['exercise_id'], sub['id'], REWARD_COINS, now))
        if cur.rowcount == 1:
            db.execute("UPDATE users SET coins=coins+? WHERE id=?", (REWARD_COINS, sub['user_id']))
        db.execute("INSERT INTO user_exercise_progress(user_id,exercise_id,status,best_score,attempts,completed_at) "
                   "VALUES(?,?,'completed',100,1,?) "
                   "ON CONFLICT(user_id,exercise_id) DO UPDATE SET status='completed',best_score=MAX(best_score,100),attempts=attempts+1,completed_at=COALESCE(completed_at,excluded.completed_at)",
                   (sub['user_id'], sub['exercise_id'], now))
    else:
        db.execute("INSERT INTO user_exercise_progress(user_id,exercise_id,status,best_score,attempts) VALUES(?,?,'in_progress',0,1) ON CONFLICT(user_id,exercise_id) DO UPDATE SET attempts=attempts+1",
                   (sub['user_id'], sub['exercise_id']))
    db.commit()


def evaluate(db: sqlite3.Connection, sub: dict, case_dir: pathlib.Path, compiler: str = "g++") -> None:
    if sub["language"] != "cpp":
        finish(db, sub, "internal_error", 0, "Unsupported language; only cpp is enabled.")
        return
    source = sub["source_code"]
    if not source or len(source.encode("utf-8")) > MAX_SOURCE_BYTES or "\x00" in source:
        finish(db, sub, "internal_error", 0, "Stored source failed validation.")
        return
    try:
        cases = load_cases(case_dir, int(sub["exercise_id"]))
        for index, case in enumerate(cases, 1):
            status, output = run_local(source, case, compiler)
            if status != "ok":
                finish(db, sub, status, 0, f"Test {index}: {output}")
                return
            if normalize_output(output) != normalize_output(case["expectedOutput"]):
                finish(db, sub, "wrong_answer", 0, f"Output mismatch on test {index}.")
                return
        finish(db, sub, "accepted", 100, f"Accepted: {len(cases)} test case(s) passed.")
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        finish(db, sub, "internal_error", 0, str(exc))


def main() -> int:
    parser = argparse.ArgumentParser(description="Virtual Pet C++ submission evaluator")
    parser.add_argument("--db", required=True, help="Same SQLite DB path used by backend")
    parser.add_argument("--case-dir", default=str(pathlib.Path(__file__).parent / "testcases"))
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--poll-seconds", type=float, default=1.0)
    parser.add_argument("--once", action="store_true", help="Process at most one queued submission")
    args = parser.parse_args()
    if args.poll_seconds < 0.2:
        parser.error("--poll-seconds must be >= 0.2")
    db = connect(args.db)
    ensure_schema(db)
    processed = False
    try:
        while True:
            sub = claim_one(db)
            if sub:
                processed = True
                try:
                    evaluate(db, sub, pathlib.Path(args.case_dir), args.compiler)
                except Exception as exc:
                    finish(db, sub, "internal_error", 0, f"Evaluator internal error: {type(exc).__name__}")
                if args.once:
                    break
            else:
                if args.once:
                    break
                time.sleep(args.poll_seconds)
    finally:
        db.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
