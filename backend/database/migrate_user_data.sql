PRAGMA foreign_keys = OFF;
BEGIN IMMEDIATE;

CREATE TABLE users_new (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT COLLATE NOCASE UNIQUE,
    email TEXT COLLATE NOCASE UNIQUE,
    password_salt TEXT,
    password_hash TEXT,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    last_login_at TEXT,
    deleted_at TEXT,
    CHECK (
        (deleted_at IS NULL
            AND username IS NOT NULL
            AND length(username) BETWEEN 3 AND 20
            AND username NOT GLOB '*[^A-Za-z0-9_]*'
            AND password_salt IS NOT NULL
            AND length(password_salt) > 0
            AND password_hash IS NOT NULL
            AND length(password_hash) > 0)
        OR
        (deleted_at IS NOT NULL
            AND username IS NULL
            AND email IS NULL
            AND password_salt IS NULL
            AND password_hash IS NULL)
    )
);

INSERT INTO users_new (id, username, email, password_salt, password_hash, created_at, last_login_at)
SELECT id, 'user_' || id, NULLIF(email, ''), password_salt, password_hash,
       COALESCE(created_at, strftime('%Y-%m-%dT%H:%M:%fZ', 'now')), last_login_at
FROM users;

CREATE TABLE player_profiles_new (
    user_id INTEGER PRIMARY KEY REFERENCES users_new(id) ON DELETE RESTRICT,
    display_name TEXT NOT NULL DEFAULT 'Player',
    coins INTEGER NOT NULL DEFAULT 100 CHECK (coins >= 0)
);

INSERT INTO player_profiles_new (user_id, coins)
SELECT id, coins FROM users;

DROP TABLE users;
ALTER TABLE users_new RENAME TO users;
ALTER TABLE player_profiles_new RENAME TO player_profiles;

DROP TABLE sessions;
CREATE TABLE sessions (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    expires_at INTEGER NOT NULL,
    revoked_at TEXT
);

CREATE INDEX sessions_user_id_idx ON sessions(user_id);

CREATE TABLE login_attempts (
    username TEXT PRIMARY KEY COLLATE NOCASE,
    failed_attempts INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
    locked_until INTEGER NOT NULL DEFAULT 0
);

COMMIT;
PRAGMA foreign_keys = ON;
