# INT-03 — API contract

**Status:** target design for frontend and backend implementation; gameplay routes are not implemented yet.  
**Version:** `v1` · **Base URL:** `/api` · **Format:** UTF-8 JSON.

This document is the single contract for the web client and the HTTP server. The
server is authoritative for coins, item prices, inventory, pet attributes, and
game rules. The client must never calculate or submit an authoritative balance,
stat, level, price, or user id.

## 1. Common rules

## 1A. HTTP server and request flow

Use one `cpp-httplib` HTTP server process to serve both the static `frontend/`
directory and `/api/*` routes from the same origin. This avoids browser CORS
configuration and lets the session cookie work naturally. The server binds to
`127.0.0.1` for local development; a public deployment must use HTTPS behind a
TLS-terminating reverse proxy.

```text
Browser -> cpp-httplib router -> session middleware -> per-user Game -> SQLite
                 |                    |                   |
                 +-- static frontend  +-- user id         +-- GameSnapshot
```

The auth handlers call `AuthService`. A protected game handler validates the
`vp_session` cookie with `AuthService::currentSession`, obtains the authenticated
user id, loads that user's `GameSnapshot` from the persistence layer, imports it
into a `Game`, and calls only public `Game` methods. HTTP handlers must not call
`Pet`, `Shop`, `User`, or `Inventory` directly to mutate game state. They map
`ActionResult` to the documented HTTP status/error code, export a snapshot,
persist it, then serialize the updated state. If validation, the action, or the
database transaction fails, the handler returns an error and leaves persisted
state unchanged.

The session cookie identifies the account; it is never used as a game-state
key. The server derives `userId` exclusively from the validated session and
must ignore any user id sent by the browser. For each request, load and save the
game under a per-user lock (or equivalent transaction/serialization mechanism)
so two concurrent actions cannot overwrite one another. Do not keep a separate
mutable in-memory `Game` cache in v1; load from SQLite and persist per request.
This makes server restarts safe and avoids cache invalidation between workers.

SQLite persistence maps `GameSnapshot` fields to the canonical account/game
tables. Load creates a default game only when the authenticated account has no
game rows. Save updates profile, pet, inventory, equipment, and `last_updated`
atomically. Store a UTC timestamp so `GET /game/state` can compute offline time;
call `Game::start()`, apply elapsed time via `updateOverTime(seconds)`, and save
the resulting snapshot before returning. The API contract's `experience` field
is reserved for the game model; until the core snapshot supports experience,
return `0` and do not accept it from clients.

Register routes before mounting `/` as the static frontend. Set a small JSON
payload limit, reject malformed/non-object JSON, set `Cache-Control: no-store`
for authenticated state and auth responses, and return JSON for all `/api/*`
errors. Static assets may use normal caching. The frontend calls relative paths
such as `/api/auth/session`, `/api/shop`, and `/api/game/state`.

- All request bodies use `Content-Type: application/json` and all API responses
  use `application/json; charset=utf-8`.
- Names use `camelCase`; identifiers and quantities are JSON integers.
- `GET` endpoints do not change game state. Game actions use `POST`.
- An unauthenticated protected endpoint returns `401` and does not disclose
  whether another user's resource exists.
- API error `code` values are stable `UPPER_SNAKE_CASE`. `message` is for display
  only; clients must use `code` for behavior.
- Unknown fields are ignored. Missing, wrongly typed, out-of-range, or otherwise
  invalid required fields return `400 INVALID_ARGUMENT`.

### Response envelopes

Successful reads return their documented object or array. Successful actions
return an `ActionResponse`:

```json
{
  "success": true,
  "code": "OK",
  "message": "Bread fed.",
  "levelsGained": 0,
  "state": { "...": "current authenticated game state" }
}
```

Every error has this shape:

```json
{
  "success": false,
  "code": "NOT_ENOUGH_COINS",
  "message": "Not enough coins for this quantity."
}
```

## 2. Authentication and session

Authentication uses one **HttpOnly session cookie**, `vp_session`. The browser
must send requests with `credentials: "same-origin"`; clients must not store a
session token in `localStorage` and must not use `Authorization: Bearer`.

Cookie attributes in local development: `Path=/; HttpOnly; SameSite=Strict;
Max-Age=604800`. Production additionally requires `Secure` over HTTPS.

### `POST /auth/register`

Creates an account and its initial game data. Body:

```json
{ "username": "player_01", "password": "at-least-8-characters" }
```

`username` is 3–20 ASCII letters, digits, or underscores and is unique
case-insensitively. `password` has at least 8 Unicode characters.

Success: `201`

```json
{
  "success": true,
  "code": "REGISTERED",
  "message": "Account created.",
  "user": { "id": 42, "username": "player_01" }
}
```

Errors: `400 INVALID_USERNAME`, `400 INVALID_PASSWORD`, `409 USERNAME_TAKEN`,
or `500 REGISTRATION_FAILED`.

### `POST /auth/login`

Body is identical to registration. On success (`200`), sets `vp_session` and
returns:

```json
{ "success": true, "code": "AUTHENTICATED", "user": { "id": 42, "username": "player_01" } }
```

Errors: `400 INVALID_ARGUMENT`, `401 INVALID_CREDENTIALS`, `429 RATE_LIMITED`,
or `500 LOGIN_FAILED`. Never reveal whether the username exists.

### `GET /auth/session`

Returns `200` for both states:

```json
{ "success": true, "authenticated": true, "user": { "id": 42, "username": "player_01" } }
```

When logged out, `authenticated` is `false` and `user` is `null`.

### `POST /auth/logout`

Revokes the current session, expires the cookie, and returns `200`:

```json
{ "success": true, "code": "LOGGED_OUT", "message": "Signed out." }
```

It is idempotent: no cookie also returns success.

## 3. Shared data types

### `GameState`

Returned by `/game/state` and after every state-changing game endpoint.

```json
{
  "user": { "id": 42, "username": "player_01", "coins": 85 },
  "pet": {
    "id": 1,
    "name": "UIA",
    "level": 1,
    "health": 100,
    "experience": 0,
    "happiness": 85,
    "hunger": 30,
    "energy": 70
  },
  "inventory": [{ "itemId": 1, "quantity": 3 }],
  "equipment": { "headwear": null, "background": 8, "floor": null }
}
```

All pet stats are integers from 0 to 100; `hunger` means fullness (`100` is
full). `level >= 1`, `coins >= 0`, and food quantities are positive. A
decoration has quantity one and may occur at most once in inventory.

### `CatalogItem`

```json
{
  "id": 1,
  "name": "Bread",
  "description": "A filling snack at a friendly price.",
  "category": "food",
  "filter": "food",
  "price": 5,
  "image": "assets/items/banh_mi.png",
  "hungerRestore": 30,
  "happinessRestore": 0
}
```

`category` is `food` or `decoration`. Decoration `filter` is one of
`headwear`, `background`, or `floor`; it has no restore fields. Image paths are
relative to the frontend root. `price` and restore values are informational;
the server always uses its own catalog values.

## 4. Game endpoints

All endpoints in this section require an authenticated session.

### Endpoint-to-`Game` mapping

The HTTP layer is an adapter. It authenticates and loads the user's game,
validates/parses the request, calls the indicated `Game` method, maps its
`ActionResult`, and serializes `exportSnapshot()` as `state`.

| HTTP endpoint | `Game` call | Request fields | Notes |
|---|---|---|---|
| `GET /game/state` | load snapshot → `importSnapshot()` → `start()` → `updateOverTime(elapsedSeconds)` → `exportSnapshot()` | none | Persist offline decay and return state. |
| `GET /shop` | read catalog from the loaded/default `Game` shop | none | This is a read-only catalog operation; may be public. |
| `POST /shop/purchases` | `buy(itemId, quantity)` | `itemId`, `quantity` | Returns `ActionResult`. |
| `POST /pet/feed` | `feed(foodId)` | `itemId` | Exactly one food is consumed by `Game`. |
| `POST /pet/play` | `play()` | none | Game applies energy/hunger costs. |
| `POST /pet/sleep` | `sleep()` | none | Game applies energy recovery. |
| `POST /pet/bath` | `bath()` | none | Game applies bath behavior. |
| `POST /pet/decorate` | `decorate(itemId)` | `itemId` | Equips owned decoration into its defined slot. |
| `POST /rewards/claim` | `claimReward(reward)` | `rewardId` | Reward contents come from server-side reward definition, never client JSON. |
| internal timer/load only | `updateOverTime(elapsedSeconds)` | server-calculated | Never accept elapsed time from the browser. |

`start`, `pause`, `resume`, `end`, `initialize`, `importSnapshot`, and
`exportSnapshot` are lifecycle/persistence operations, not browser actions in
v1. The server starts a loaded game for each request and ends its request-local
instance after saving. The server must never expose `initialize` to an ordinary
client request because it resets the game.

### `GET /game/state`

Returns `200` and a `GameState`. The server applies elapsed-time decay before
building the response, then persists the resulting state atomically.

### `GET /shop`

Returns `200` and an array of `CatalogItem`. It is public so the shop can be
previewed before login; prices are not user-specific in v1.

### `POST /shop/purchases`

Body:

```json
{ "itemId": 1, "quantity": 3 }
```

`quantity` must be an integer from 1 to 999. For decorations it must be 1.
Success: `200 ActionResponse` with the updated `state`.

Errors: `400 INVALID_ARGUMENT`, `404 ITEM_NOT_FOUND`, `409 ALREADY_OWNED`,
`409 NOT_ENOUGH_COINS`, or `401 UNAUTHENTICATED`.

### `POST /pet/feed`

Body: `{ "itemId": 1 }`. Consumes exactly one owned food item and returns
`200 ActionResponse` with updated state. `levelsGained` is the number of levels
earned from this action.

Errors: `400 INVALID_ARGUMENT`, `400 NOT_FOOD`, `404 ITEM_NOT_FOUND`,
`409 NOT_OWNED`, or `401 UNAUTHENTICATED`.

### `POST /pet/play`, `POST /pet/sleep`, `POST /pet/bath`

These endpoints take an empty body (or no body) and return `200
ActionResponse` with updated state. `play` and `bath` can report
`levelsGained`.

Errors: `409 PET_TOO_TIRED` for play with no energy, `409 ENERGY_FULL` for
sleep with full energy, `409 INVALID_STATE` when the game is unavailable, or
`401 UNAUTHENTICATED`.

### `POST /pet/decorate`

Body:

```json
{ "itemId": 6 }
```

The endpoint calls `Game::decorate(itemId)`. The item's catalog definition
determines its slot; the game replaces any currently equipped decoration in
that slot. Returns `200 ActionResponse`.

Errors: `400 INVALID_ARGUMENT`, `400 NOT_DECORATION`, `404 ITEM_NOT_FOUND`,
`409 NOT_OWNED`, `409 INVALID_STATE`, or `401 UNAUTHENTICATED`.

`Game` currently has no rename or unequip method. Those are outside this
contract until implemented through an explicit validated `Game` API; HTTP
handlers must not mutate `Pet` directly to emulate them.

### `POST /rewards/claim`

Body: `{ "rewardId": "daily_login" }`. The server resolves this ID against
its reward definition and calls `Game::claimReward(reward)`. The client cannot
choose reward coins or items. Success is `200 ActionResponse`.

Errors: `400 INVALID_ARGUMENT`, `404 REWARD_NOT_FOUND`, `409 ALREADY_CLAIMED`,
`409 INVALID_STATE`, or `401 UNAUTHENTICATED`.

## 5. Status and error mapping

| HTTP | Code | Meaning |
|---:|---|---|
| 400 | `INVALID_ARGUMENT` | Invalid JSON or invalid input field. |
| 400 | `NOT_FOOD`, `NOT_DECORATION` | Item exists but cannot be used for that action. |
| 401 | `UNAUTHENTICATED` | Missing, expired, or revoked session. |
| 404 | `ITEM_NOT_FOUND` | No catalog item with that ID. |
| 409 | `NOT_OWNED`, `ALREADY_OWNED`, `NOT_ENOUGH_COINS`, `PET_TOO_TIRED`, `ENERGY_FULL`, `INVALID_STATE` | Valid request conflicts with current game state. |
| 429 | `RATE_LIMITED` | Login attempts temporarily limited; include `Retry-After`. |
| 500 | `INTERNAL_ERROR` | Unexpected server/database failure; no state is partially applied. |

## 6. Atomicity and frontend implementation rules

- Each mutating endpoint validates first, then commits all changes in one
  database transaction. An error leaves coins, inventory, pet, and equipment
  unchanged.
- The client replaces its local state with `response.state`; it does not apply
  optimistic balances or stat calculations.
- The client obtains the catalog from `GET /api/shop`; it must not maintain a
  second hard-coded catalog.
- The authenticated UI initializes in this order: `GET /api/auth/session`,
  then `GET /api/shop`, then `GET /api/game/state` when authenticated.
- `/api/me`, `/api/pet`, `/api/logout`, Bearer tokens, and localStorage game
  persistence are legacy interfaces and are not part of v1. They must not be
  used by new frontend or backend work.

## 7. Acceptance checklist

- Frontend mock data implements every endpoint and response type above.
- Backend tests cover every success path and every listed error code.
- An integration test verifies the client can register, log in, fetch state,
  purchase, feed, equip, log out, and restore the same persisted state after a
  server restart.
- Any contract change is made in this document first and is versioned or
  backward-compatible.
