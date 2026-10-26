# C++ evaluator worker (SUB-04)

The HTTP server persists source submissions as `queued`. This worker claims queued rows,
compiles/runs C++17 with a local compiler, compares stdout to configured test cases, stores
the final status and score, and awards 10 coins only on the user's first accepted submission
for an exercise. Docker is not required.

## Requirements

- Python 3.10+ (standard library only)
- C++17 compiler available as `g++` or configured with `--compiler` / `CXX`
- The exact same SQLite file path used by the HTTP server

## Start

From the repository root, start the backend as usual, then in a second terminal:

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db
```

If `g++` is not on PATH:

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db --compiler C:/path/to/g++.exe
```

Process only one submission (useful for local smoke tests):

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db --once
```

Run one worker instance per database. On restart, rows left in `evaluating` are re-queued.

## Local execution controls

Each submission is compiled in a temporary directory and executed as a separate process.
The worker applies a per-test wall-clock timeout, kills the process when it exceeds the
time limit, caps captured output at 64 KiB, and rejects source larger than 32 KiB. Compile
errors and non-zero exit codes are reported separately.

The local evaluator is intended for an internal student project/demo. It is **not a security
sandbox for arbitrary hostile code** because the evaluated process still runs on the host OS.
For a public judge, add a VM/container/OS-level sandbox before exposing submissions to
untrusted users.

## Test cases

JSON files are named `<exercise_id>.json` under `testcases/`. Each is a non-empty array:

```json
[
  {"input":"7 5\n", "expectedOutput":"12\n", "hidden":true,
   "timeLimitMs":2000, "memoryLimitMb":128}
]
```

The current set contains 22 cases across the five seeded exercises, including public and
hidden cases plus edge cases.

## Worker tests

```powershell
python -m unittest discover -s backend/evaluator/tests -v
```

Current result: **9 tests passed, 0 skipped**. Tests cover output normalization, testcase
configuration, compile errors, infinite-loop timeout, output limit and a correct submission.
