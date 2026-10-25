# C++ evaluator worker (SUB-04)

The HTTP server persists source submissions as `queued`. This worker claims queued rows,
compiles/runs C++ in a disposable Docker container, compares stdout to configured test
cases, stores the final status and score, and awards 10 coins only on the user's first
accepted submission for an exercise.

## Requirements

- Python 3.10+ (standard library only)
- Docker Engine running, with permission to run containers
- Pullable image `gcc:13-bookworm` (override with `PET_JUDGE_IMAGE` if your deployment
  maintains an audited equivalent)
- The exact same SQLite file path used by the HTTP server

## Start

From the repository root, start the backend as usual, then in a second terminal:

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db
```

Use the absolute database path if the server was started with a custom path. Do not point
the worker at a different database, or submissions will remain queued.

Process only one submission (useful for local smoke tests):

```powershell
python backend/evaluator/worker.py --db backend/build/virtual_pet.db --once
```

The first run requires Docker to have the judge image available. Run one worker instance per database; on restart it re-queues jobs left in `evaluating`. The worker does not
silently execute source code on the host. If Docker is unavailable, the submission is
marked `internal_error` rather than executed unsafely.

## Sandbox controls

Each test gets a new container with networking disabled, read-only root filesystem, dropped
Linux capabilities, `no-new-privileges`, an unprivileged UID, PID/CPU/memory limits, bounded
tmpfs, file-size/file-descriptor limits and an execution timeout. The source is mounted
read-only. Review Docker Engine hardening and OS-level isolation before exposing this to
untrusted public traffic; a dedicated VM/runtime such as gVisor or Kata is recommended for
higher-risk production deployments.

## Test cases

JSON files are named `<exercise_id>.json` under `testcases/`. Each is a non-empty array:

```json
[
  {"input":"7 5\n", "expectedOutput":"12\n", "hidden":true,
   "timeLimitMs":2000, "memoryLimitMb":128}
]
```

Keep hidden case inputs and expected output on the server only. The current sample cases
match the seeded exercise descriptions; replace/extend them to match the actual curriculum.

## Worker tests

```powershell
python -m unittest discover -s backend/evaluator/tests -v
```

These tests cover output normalization and test-case configuration. End-to-end sandbox tests
must be run on a host with Docker available; they are not claimed as passed by this package.
