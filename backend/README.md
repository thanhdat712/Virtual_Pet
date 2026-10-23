Local Windows backend

Build from the repository root with .\backend\build.ps1 (MinGW GCC 6.3,
no CMake needed). Use .\backend\build.ps1 -Run -Port 8080 to launch.
If that executable is already running, leave it in place and build separately:
.\backend\build.ps1 -BuildDir backend/build-feed
Use a sibling directory under backend so the static root still resolves correctly.
Alternatively, from backend/build, run .\server.exe 8080 "C:\path\test.db".
Arguments are port, then optional SQLite database path. The default database
is virtual_pet.db in the working directory. Keep the working directory at
backend/build so the static root resolves to ../../frontend.

GET / serves the existing frontend. GET /js/api.js and other allowed static
assets use that same root. Traversal, percent-encoded paths, backslashes and
colons are rejected. No frontend files are modified by this implementation.
The frontend owner must use location.origin as the API base when using HTTP.

All bodies and responses are JSON except static files and OPTIONS responses.
Authentication uses Authorization: Bearer <token>. Errors return {"error":...}.

Public contracts

POST /api/register {email,password} -> 201 {token,user}.
POST /api/login {email,password} -> 200 {token,user}; invalid credentials -> 401.
Login verification permits 10 credential failures per rolling 30 seconds across
the local process, including unknown emails. Further login attempts return 429
with Retry-After seconds before password hashing. Successful logins and malformed
requests do not consume this budget; successes do not erase earlier failures.
The queue holds at most 10 timestamps, uses a monotonic clock, and resets on
restart. Cooldown temporarily blocks even valid logins, but existing sessions,
registration, and other API requests remain available and are not rate limited.
Registration requires an email containing @ and a password of at least 6 bytes.
Email is limited to 254 bytes and password to 1024 bytes; matching is case-sensitive.
GET /api/shop -> array of 13 items, each with id, name, description, category,
categoryLabel, filter, price, image, icon; values match frontend/js/app.js.
Food also includes server-owned hungerRestore and happinessRestore values.
GET /api/exercises -> array of {id,title,description,difficulty,maxScore}.
IDs 1..5 ask for Hello World!, 7+5, parity of 14, maximum of 3 9 2 7,
and ascending sort of 5 1 4 2 3, respectively. Expected answers are
Hello World!, 12, even, 9, and 1 2 3 4 5. Whitespace is normalized;
case and punctuation remain significant. No submitted code is executed.

Authenticated contracts

GET /api/me -> {user,pet,progress,inventory}.
user is {id,email,coins}; pet is {name,level,experience,happiness,hunger,energy}.
inventory is [{id,quantity}]; a new user starts with 100 coins and no inventory.
progress is [{exerciseId,status,bestScore,attempts,completedAt}].
An incomplete exercise has an empty completedAt string.

POST /api/shop/purchases {itemId,quantity} -> 200 {user,inventory}.
itemId and quantity must be JSON integers; quantity is required, 1..1000.
Authoritative price, coin debit, inventory addition and purchase history are
committed in one SQLite transaction. Client price and userId are ignored.
Food stacks to a maximum of 1000000 units per item. Decorations require
quantity 1 and may only be owned once. Invalid quantity -> 400, unknown
item -> 404, duplicate decoration / insufficient funds / inventory limit -> 409.
Failed purchases do not charge coins or add purchase history.

POST /api/pet/feed {itemId} -> 200 {user,pet,inventory}, using the same
user, pet and inventory formats as GET /api/me. itemId must be a positive
32-bit JSON integer. One owned food is consumed per request; the last unit
deletes the inventory row. Consumption and pet updates commit together or
roll back together. Hunger means fullness: feeding increases hunger and
happiness, each clamped to 0..100. Food is consumed even at full stats.
Restore values (hunger/happiness), matching Shop.cpp: Bread 30/0, Apple 20/5,
Golden Apple 40/15, Cream Cake 10/20, Honey 5/25.
Invalid itemId or nonfood -> 400, unknown item -> 404, unowned/depleted food
-> 409. Client userId, quantity, restores, coins and stats are ignored.
Only the authenticated user's inventory and pet change; coins are unchanged.

PUT /api/pet {name} -> 200 {user,pet}. Only name is accepted (1..80 UTF-8 bytes).
PUT /api/pet/species {species} -> 200 {user,pet}. species is "cat" or "dog".
Any coins, level, experience, happiness, hunger or energy field causes 400
and no update; mixed name/stat requests are also rejected.

POST /api/exercises/{id}/submissions {answer} -> 201 {score,status,progress}.
answer is a string of at most 8192 bytes; empty answers grade 0.
score is always 0 or 100, determined on the server. Submitted score/status
fields are ignored. The response status describes this attempt; progress
retains completed status, bestScore 100 and first completedAt after later
failures. Each accepted attempt increments attempts and stores the answer.
Unknown exercise -> 404, missing/non-string/oversized answer -> 400.

POST /api/logout -> 200 {ok:true}; revokes the bearer session persistently.
Missing, expired or revoked tokens on protected routes return 401.

Persistence and local security

SQLite WAL, prepared statements and BEGIN IMMEDIATE transactions are used.
Tables include users, pets, sessions, exercises, submissions,
user_exercise_progress, inventory and purchase_transactions. Existing databases
gain sessions.expires_at and submissions.answer automatically. Old sessions
without expiry are invalidated. Deterministic exercise descriptions are seeded
on startup. No balance, pet level or stat changes are awarded by submissions.

Passwords use Windows CNG PBKDF2-HMAC-SHA256, 600000 iterations, 16 random
bytes represented as hex salt (the stored hex text is the PBKDF2 salt input),
and a 32-byte derived key. CNG functions are dynamically resolved from
bcrypt.dll for old MinGW headers. Hash comparison uses a constant-time loop.
Legacy SHA256(salt + password) migrates with a fresh salt only after a
successful login. Tokens are 32 random bytes represented as hex, stored only
as SHA256 hashes, and expire after seven days without sliding renewal.

The listener binds only 127.0.0.1. Host must be localhost:<port> or
127.0.0.1:<port>. Allowed browser origins are http://localhost:5500,
http://127.0.0.1:5500, file origin null and the server's own HTTP origin.
Other origins return 403. CORS never uses a wildcard. Local file origin is
allowed explicitly for this workflow, so this is a local development service,
not an Internet-facing authentication deployment; no TLS. The failed-login
budget is a local safeguard, not comprehensive resource-abuse protection;
successful registrations and logins can still perform unthrottled PBKDF2 work.

Request headers are limited to 16384 bytes and body to 32768 bytes, with
three-second receive/send inactivity timeouts. Incomplete or ambiguous framing
returns 400; unsupported transfer encoding and duplicate lengths are rejected.
Header limit returns 431 and body limit returns 413. JSON nesting is limited
to 16 levels. The single-threaded server closes connections after each response.

JSON dependency

third_party/nlohmann/json.hpp is the pinned upstream single header v3.11.3:
https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp
Its upstream license text is retained in the header. All request parsing and
response serialization use this library, not the former homemade parser.

Automated verification

Run .\backend\tests\api_integration.ps1 from the repository root. Use
-SkipBuild to reuse a freshly compiled server. The suite compiles a test-only
SQLite probe, allocates loopback ports, starts servers with a disposable DB,
tests HTTP contracts and stored state, then stops only its own processes and
deletes its temporary DB and executables, including on failure.
To test the separate build without stopping the live server:
.\backend\tests\api_integration.ps1 -SkipBuild -BuildDir backend/build-feed
Omit -SkipBuild to compile that directory first. Feeding checks cover all five
restores, input validation, depletion, clamps, unchanged coins, user isolation,
rollback on a forced SQLite failure, and persistence across restart.
