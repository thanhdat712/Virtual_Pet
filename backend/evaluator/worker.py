#!/usr/bin/env python3
"""Docker-backed C++ submission worker. Requires Docker Engine and Python 3.10+.
Run beside the backend with the same SQLite database path as the HTTP server.
"""
from __future__ import annotations
import argparse, json, os, pathlib, sqlite3, subprocess, tempfile, time, math
from datetime import datetime, timezone
IMAGE = os.environ.get("PET_JUDGE_IMAGE", "gcc:13-bookworm")
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
    # A single worker is supported. On restart, release claims abandoned by its prior process.
    db.execute("UPDATE code_submissions SET status='queued',message='Recovered after evaluator restart.' WHERE status='evaluating' AND finished_at=''")
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
        if not isinstance(case, dict) or not isinstance(case.get("input", ""), str) or not isinstance(case.get("expectedOutput"), str):
            raise ValueError(f"Invalid test case at index {index}")
        timeout = int(case.get("timeLimitMs", DEFAULT_TIMEOUT_MS))
        memory = int(case.get("memoryLimitMb", DEFAULT_MEMORY_MB))
        if not 100 <= timeout <= 10000 or not 16 <= memory <= 512:
            raise ValueError(f"Invalid resource limits at index {index}")
        cases.append({"input": case.get("input", ""), "expectedOutput": case["expectedOutput"],
                      "timeLimitMs": timeout, "memoryLimitMb": memory})
    return cases


def normalize_output(value: str) -> str:
    return "\n".join(line.rstrip(" \t\r") for line in value.replace("\r\n", "\n").replace("\r", "\n").split("\n")).rstrip("\n")


def run_in_docker(source: str, case: dict, docker_bin: str) -> tuple[str, str]:
    """Compile and execute in a disposable container. Returns (status, output/message)."""
    with tempfile.TemporaryDirectory(prefix="pet-judge-") as temp:
        source_dir = pathlib.Path(temp)
        source_path = source_dir / "main.cpp"
        source_path.write_text(source, encoding="utf-8")
        source_path.chmod(0o644)
        # The only host mount is the source file, read-only. Root FS is read-only;
        # /tmp is bounded tmpfs and is the only writable executable location.
        command = [docker_bin, "run", "--rm", "-i", "--network", "none", "--read-only",
                   "--cap-drop", "ALL", "--security-opt", "no-new-privileges:true", "--user", "65534:65534",
                   "--pids-limit", "64", "--memory", "512m",
                   "--memory-swap", "512m", "--cpus", "1.0",
                   "--ulimit", "cpu=3:3", "--ulimit", "fsize=65536:65536",
                   "--ulimit", "nofile=64:64", "--tmpfs", "/tmp:rw,nosuid,nodev,size=64m,exec",
                   "-v", f"{source_path}:/workspace/main.cpp:ro", "-w", "/workspace", IMAGE,
                   "bash", "-o", "pipefail", "-lc", f"g++ -std=c++17 -O2 -pipe -static-libstdc++ -static-libgcc /workspace/main.cpp -o /tmp/main 2>/tmp/compile.err || {{ cat /tmp/compile.err; exit 42; }}; ulimit -v {case['memoryLimitMb'] * 1024}; timeout --kill-after=0.2s {max(1, math.ceil(case['timeLimitMs'] / 1000))}s /tmp/main 2>&1 | head -c 65537"]
        try:
            proc = subprocess.run(command, input=case["input"].encode("utf-8"), stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, timeout=case["timeLimitMs"] / 1000 + 8,
                                  check=False, cwd=temp)
        except subprocess.TimeoutExpired:
            return "time_limit_exceeded", "Program exceeded the time limit."
        out = proc.stdout[:MAX_OUTPUT_BYTES + 1]
        err = proc.stderr[:MAX_OUTPUT_BYTES + 1]
        if len(out) > MAX_OUTPUT_BYTES or len(err) > MAX_OUTPUT_BYTES:
            return "runtime_error", "Output exceeded 64 KiB limit."
        stderr = err.decode("utf-8", errors="replace")
        stdout = out.decode("utf-8", errors="replace")
        if proc.returncode == 42:
            return "compile_error", stdout[-4000:] or stderr[-4000:] or "Compilation failed."
        if proc.returncode == 124:
            return "time_limit_exceeded", "Program exceeded the time limit."
        if proc.returncode == 137:
            return "memory_limit_exceeded", "Program was killed after exceeding the container resource limit."
        if proc.returncode == 125 or "Cannot connect to the Docker daemon" in stderr:
            return "internal_error", "Docker Engine is unavailable or failed to start the sandbox."
        if proc.returncode != 0:
            # Docker daemon/runner failures should not be mistaken for user runtime errors.
            if "Cannot connect to the Docker daemon" in stderr or "docker: command not found" in stderr:
                return "internal_error", "Docker Engine is unavailable; evaluator cannot run safely."
            return "runtime_error", f"Program exited with code {proc.returncode}. {stderr[-1000:]}".strip()
        return "ok", stdout


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


def evaluate(db: sqlite3.Connection, sub: dict, case_dir: pathlib.Path, docker_bin: str) -> None:
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
            status, output = run_in_docker(source, case, docker_bin)
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
    parser.add_argument("--docker", default="docker")
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
                    evaluate(db, sub, pathlib.Path(args.case_dir), args.docker)
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
    return 0 if processed or not args.once else 0

if __name__ == "__main__":
    raise SystemExit(main())
