PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL COLLATE NOCASE UNIQUE,
    email TEXT NOT NULL UNIQUE,
    password_salt TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    email_verified INTEGER NOT NULL DEFAULT 0 CHECK (email_verified IN (0, 1)),
    coins INTEGER NOT NULL DEFAULT 100 CHECK (coins >= 0),
    sound_enabled INTEGER NOT NULL DEFAULT 1 CHECK (sound_enabled IN (0, 1)),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    last_login_at TEXT
);

CREATE TABLE IF NOT EXISTS pets (
    user_id INTEGER PRIMARY KEY REFERENCES users(id),
    name TEXT NOT NULL DEFAULT 'UIA',
    species TEXT NOT NULL DEFAULT 'cat' CHECK (species IN ('cat', 'dog')),
    species_chosen INTEGER NOT NULL DEFAULT 0 CHECK (species_chosen IN (0, 1)),
    level INTEGER NOT NULL DEFAULT 1 CHECK (level >= 1),
    happiness INTEGER NOT NULL DEFAULT 0 CHECK (happiness BETWEEN 0 AND 100),
    hunger INTEGER NOT NULL DEFAULT 50 CHECK (hunger BETWEEN 0 AND 100),
    energy INTEGER NOT NULL DEFAULT 50 CHECK (energy BETWEEN 0 AND 100),
    sleep_until INTEGER NOT NULL DEFAULT 0 CHECK (sleep_until >= 0)
);

CREATE TABLE IF NOT EXISTS sessions (
    token TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    expires_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS password_reset_tokens (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS email_verification_tokens (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS pending_email_changes (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    email TEXT NOT NULL UNIQUE COLLATE NOCASE,
    token_hash TEXT NOT NULL UNIQUE,
    expires_at INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS exercises (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    difficulty TEXT NOT NULL DEFAULT 'easy',
    max_score INTEGER NOT NULL DEFAULT 100
);

CREATE TABLE IF NOT EXISTS submissions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id),
    exercise_id INTEGER NOT NULL REFERENCES exercises(id),
    score INTEGER NOT NULL DEFAULT 0,
    status TEXT NOT NULL DEFAULT 'submitted',
    submitted_at TEXT NOT NULL DEFAULT (datetime('now')),
    answer TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS user_exercise_progress (
    user_id INTEGER NOT NULL,
    exercise_id INTEGER NOT NULL,
    status TEXT NOT NULL DEFAULT 'in_progress',
    best_score INTEGER NOT NULL DEFAULT 0,
    attempts INTEGER NOT NULL DEFAULT 0,
    completed_at TEXT,
    PRIMARY KEY (user_id, exercise_id)
);

CREATE TABLE IF NOT EXISTS study_state (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    total_points INTEGER NOT NULL DEFAULT 0 CHECK (total_points >= 0),
    happiness_remainder_points INTEGER NOT NULL DEFAULT 0 CHECK (happiness_remainder_points BETWEEN 0 AND 49),
    elapsed_remainder_seconds INTEGER NOT NULL DEFAULT 0 CHECK (elapsed_remainder_seconds BETWEEN 0 AND 104),
    is_active INTEGER NOT NULL DEFAULT 0 CHECK (is_active IN (0, 1)),
    last_activity_at INTEGER
);

CREATE TABLE IF NOT EXISTS study_stage_progress (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    stage TEXT NOT NULL CHECK (stage IN (
        'A1-easy','A1-medium','A1-hard','A2-easy','A2-medium','A2-hard',
        'B1-easy','B1-medium','B1-hard','B2-easy','B2-medium','B2-hard',
        'C1-easy','C1-medium','C1-hard','C2-easy','C2-medium','C2-hard'
    )),
    stage_order INTEGER NOT NULL CHECK (stage_order BETWEEN 0 AND 17),
    points_awarded INTEGER NOT NULL CHECK (points_awarded > 0),
    coins_awarded INTEGER NOT NULL DEFAULT 0 CHECK (coins_awarded >= 0),
    completed_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    PRIMARY KEY (user_id, stage),
    UNIQUE (user_id, stage_order)
);

CREATE TABLE IF NOT EXISTS practice_state (
    user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    total_points INTEGER NOT NULL DEFAULT 0 CHECK (total_points >= 0),
    happiness_remainder_points INTEGER NOT NULL DEFAULT 0 CHECK (happiness_remainder_points BETWEEN 0 AND 19),
    best_score INTEGER NOT NULL DEFAULT 0 CHECK (best_score >= 0),
    elapsed_remainder_seconds INTEGER NOT NULL DEFAULT 0 CHECK (elapsed_remainder_seconds BETWEEN 0 AND 104),
    is_active INTEGER NOT NULL DEFAULT 0 CHECK (is_active IN (0, 1)),
    last_activity_at INTEGER,
    active_game_id TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS practice_game_rewards (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    game_id TEXT NOT NULL,
    eaten INTEGER NOT NULL CHECK (eaten >= 0),
    points_awarded INTEGER NOT NULL CHECK (points_awarded >= 0),
    coins_awarded INTEGER NOT NULL CHECK (coins_awarded >= 0),
    completed_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    PRIMARY KEY (user_id, game_id)
);

CREATE TABLE IF NOT EXISTS inventory (
    user_id INTEGER NOT NULL REFERENCES users(id),
    item_id INTEGER NOT NULL CHECK (item_id BETWEEN 1 AND 13),
    quantity INTEGER NOT NULL CHECK (quantity BETWEEN 1 AND 1000000),
    PRIMARY KEY (user_id, item_id),
    CHECK (item_id BETWEEN 1 AND 5 OR quantity = 1)
);

CREATE TABLE IF NOT EXISTS purchase_transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id),
    item_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    unit_price INTEGER NOT NULL CHECK (unit_price > 0),
    total INTEGER NOT NULL CHECK (total = quantity * unit_price),
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

CREATE TABLE IF NOT EXISTS equipped_decorations (
    user_id INTEGER NOT NULL REFERENCES users(id),
    slot TEXT NOT NULL CHECK (slot IN ('headwear', 'background', 'floor')),
    item_id INTEGER NOT NULL,
    PRIMARY KEY (user_id, slot)
);

-- Source-code submissions (separate from the existing short-answer quiz history).
CREATE TABLE IF NOT EXISTS code_submissions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,
    source_code TEXT NOT NULL,
    language TEXT NOT NULL DEFAULT 'cpp',
    status TEXT NOT NULL DEFAULT 'queued',
    score INTEGER NOT NULL DEFAULT 0 CHECK (score BETWEEN 0 AND 100),
    message TEXT NOT NULL DEFAULT '',
    submitted_at TEXT NOT NULL,
    finished_at TEXT NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS code_submissions_user_idx ON code_submissions(user_id, id DESC);


-- Idempotent reward ledger: one first-accepted reward per user and exercise.
CREATE TABLE IF NOT EXISTS code_submission_rewards (
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,
    submission_id INTEGER NOT NULL REFERENCES code_submissions(id) ON DELETE CASCADE,
    coins_awarded INTEGER NOT NULL DEFAULT 10 CHECK (coins_awarded >= 0),
    awarded_at TEXT NOT NULL,
    PRIMARY KEY (user_id, exercise_id)
);
CREATE INDEX IF NOT EXISTS code_submissions_queue_idx ON code_submissions(status, id);
