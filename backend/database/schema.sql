PRAGMA foreign_keys = ON;

CREATE TABLE users (
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

CREATE TABLE player_profiles (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE RESTRICT,
    display_name TEXT NOT NULL DEFAULT 'Player',
    coins INTEGER NOT NULL DEFAULT 100 CHECK (coins >= 0)
);

CREATE TABLE pets (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE RESTRICT,
    name TEXT NOT NULL DEFAULT 'UIA',
    level INTEGER NOT NULL DEFAULT 1 CHECK (level > 0),
    experience INTEGER NOT NULL DEFAULT 0 CHECK (experience >= 0),
    happiness INTEGER NOT NULL DEFAULT 85 CHECK (happiness BETWEEN 0 AND 100),
    hunger INTEGER NOT NULL DEFAULT 30 CHECK (hunger BETWEEN 0 AND 100),
    energy INTEGER NOT NULL DEFAULT 70 CHECK (energy BETWEEN 0 AND 100)
);

CREATE TABLE inventory (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    item_id INTEGER NOT NULL CHECK (item_id BETWEEN 1 AND 13),
    quantity INTEGER NOT NULL CHECK (quantity BETWEEN 1 AND 1000000),
    PRIMARY KEY (user_id, item_id),
    CHECK (item_id BETWEEN 1 AND 5 OR quantity = 1)
);
