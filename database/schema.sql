-- ============================================================================
-- Virtual Pet Raising & C++ Learning Game - Database Schema
-- DBMS: SQLite 3
-- Charset: UTF-8
-- ============================================================================

PRAGMA foreign_keys = ON;

-- ----------------------------------------------------------------------------
-- 1. Bảng USERS: Thông tin tài khoản người chơi
-- ----------------------------------------------------------------------------
DROP TABLE IF EXISTS sessions;
DROP TABLE IF EXISTS inventory_items;
DROP TABLE IF EXISTS submissions;
DROP TABLE IF EXISTS progress;
DROP TABLE IF EXISTS pets;
DROP TABLE IF EXISTS test_cases;
DROP TABLE IF EXISTS exercises;
DROP TABLE IF EXISTS quests;
DROP TABLE IF EXISTS items;
DROP TABLE IF EXISTS users;

CREATE TABLE users (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    username        TEXT NOT NULL UNIQUE COLLATE NOCASE,
    email           TEXT UNIQUE COLLATE NOCASE,
    password_hash   TEXT NOT NULL,
    coins           INTEGER NOT NULL DEFAULT 100 CHECK (coins >= 0),
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    last_login_at   TEXT
);

-- ----------------------------------------------------------------------------
-- 2. Bảng SESSIONS: Phiên đăng nhập / Authentication Tokens
-- ----------------------------------------------------------------------------
CREATE TABLE sessions (
    token           TEXT PRIMARY KEY,
    user_id         INTEGER NOT NULL,
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    expires_at      TEXT NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------------------------
-- 3. Bảng PETS: Thú cưng thuộc quyền sở hữu của người chơi
-- ----------------------------------------------------------------------------
CREATE TABLE pets (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL UNIQUE,
    name            TEXT NOT NULL DEFAULT 'UIA',
    type            TEXT NOT NULL DEFAULT 'CAT' CHECK (type IN ('CAT', 'DOG')),
    level           INTEGER NOT NULL DEFAULT 1 CHECK (level >= 1),
    experience      INTEGER NOT NULL DEFAULT 0 CHECK (experience >= 0),
    health          INTEGER NOT NULL DEFAULT 100 CHECK (health BETWEEN 0 AND 100),
    happiness       INTEGER NOT NULL DEFAULT 85 CHECK (happiness BETWEEN 0 AND 100),
    hunger          INTEGER NOT NULL DEFAULT 30 CHECK (hunger BETWEEN 0 AND 100),
    energy          INTEGER NOT NULL DEFAULT 70 CHECK (energy BETWEEN 0 AND 100),
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    updated_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------------------------
-- 4. Bảng ITEMS: Danh mục vật phẩm (Thức ăn, Đồ trang trí)
-- ----------------------------------------------------------------------------
CREATE TABLE items (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    name                TEXT NOT NULL,
    description         TEXT NOT NULL DEFAULT '',
    category            TEXT NOT NULL CHECK (category IN ('food', 'decoration')),
    filter              TEXT NOT NULL CHECK (filter IN ('food', 'headwear', 'background', 'floor')),
    price               INTEGER NOT NULL CHECK (price >= 0),
    image_path          TEXT NOT NULL,
    icon                TEXT NOT NULL DEFAULT '',
    hunger_restore      INTEGER NOT NULL DEFAULT 0 CHECK (hunger_restore >= 0),
    happiness_restore   INTEGER NOT NULL DEFAULT 0 CHECK (happiness_restore >= 0),
    slot                TEXT CHECK (slot IN ('headwear', 'background', 'floor') OR slot IS NULL),
    is_available        INTEGER NOT NULL DEFAULT 1 CHECK (is_available IN (0, 1)),
    created_at          TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

-- ----------------------------------------------------------------------------
-- 5. Bảng INVENTORY_ITEMS: Túi đồ người chơi (Quan hệ Users - Items)
-- ----------------------------------------------------------------------------
CREATE TABLE inventory_items (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL,
    item_id         INTEGER NOT NULL,
    quantity        INTEGER NOT NULL DEFAULT 1 CHECK (quantity > 0),
    is_equipped     INTEGER NOT NULL DEFAULT 0 CHECK (is_equipped IN (0, 1)),
    acquired_at     TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    UNIQUE (user_id, item_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE RESTRICT
);

-- ----------------------------------------------------------------------------
-- 6. Bảng QUESTS: Nhiệm vụ hàng ngày & mục tiêu rèn luyện
-- ----------------------------------------------------------------------------
CREATE TABLE quests (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    title           TEXT NOT NULL,
    description     TEXT NOT NULL DEFAULT '',
    quest_type      TEXT NOT NULL DEFAULT 'DAILY' CHECK (quest_type IN ('DAILY', 'ACHIEVEMENT', 'EVENT')),
    target_action   TEXT NOT NULL CHECK (target_action IN ('FEED_PET', 'SOLVE_EXERCISE', 'LOGIN', 'BUY_ITEM')),
    target_count    INTEGER NOT NULL DEFAULT 1 CHECK (target_count > 0),
    reward_coins    INTEGER NOT NULL DEFAULT 0 CHECK (reward_coins >= 0),
    reward_exp      INTEGER NOT NULL DEFAULT 0 CHECK (reward_exp >= 0),
    reward_item_id  INTEGER,
    is_active       INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)),
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (reward_item_id) REFERENCES items(id) ON DELETE SET NULL
);

-- ----------------------------------------------------------------------------
-- 7. Bảng EXERCISES: Danh sách bài tập lập trình C/C++
-- ----------------------------------------------------------------------------
CREATE TABLE exercises (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    title           TEXT NOT NULL,
    description     TEXT NOT NULL DEFAULT '',
    difficulty      TEXT NOT NULL DEFAULT 'easy' CHECK (difficulty IN ('easy', 'medium', 'hard')),
    max_score       INTEGER NOT NULL DEFAULT 100 CHECK (max_score > 0),
    reward_coins    INTEGER NOT NULL DEFAULT 15 CHECK (reward_coins >= 0),
    reward_exp      INTEGER NOT NULL DEFAULT 20 CHECK (reward_exp >= 0),
    is_active       INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0, 1)),
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime'))
);

-- ----------------------------------------------------------------------------
-- 8. Bảng TEST_CASES: Các bộ test kiểm thử tự động cho bài tập
-- ----------------------------------------------------------------------------
CREATE TABLE test_cases (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    exercise_id     INTEGER NOT NULL,
    input_data      TEXT NOT NULL DEFAULT '',
    expected_output TEXT NOT NULL,
    is_hidden       INTEGER NOT NULL DEFAULT 0 CHECK (is_hidden IN (0, 1)),
    ordinal         INTEGER NOT NULL DEFAULT 1 CHECK (ordinal >= 1),
    created_at      TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------------------------
-- 9. Bảng SUBMISSIONS: Lịch sử nộp bài tập của người chơi
-- ----------------------------------------------------------------------------
CREATE TABLE submissions (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id         INTEGER NOT NULL,
    exercise_id     INTEGER NOT NULL,
    source_code     TEXT NOT NULL,
    status          TEXT NOT NULL DEFAULT 'submitted' 
                    CHECK (status IN ('submitted', 'evaluating', 'accepted', 'wrong_answer', 'runtime_error', 'time_limit_exceeded', 'compile_error')),
    score           INTEGER NOT NULL DEFAULT 0 CHECK (score >= 0),
    submitted_at    TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE CASCADE
);

-- ----------------------------------------------------------------------------
-- 10. Bảng PROGRESS: Tiến độ & điểm cao nhất của người chơi trên từng bài tập
-- ----------------------------------------------------------------------------
CREATE TABLE progress (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id             INTEGER NOT NULL,
    exercise_id         INTEGER NOT NULL,
    status              TEXT NOT NULL DEFAULT 'in_progress' CHECK (status IN ('not_started', 'in_progress', 'completed')),
    best_score          INTEGER NOT NULL DEFAULT 0 CHECK (best_score >= 0),
    attempts            INTEGER NOT NULL DEFAULT 0 CHECK (attempts >= 0),
    last_submitted_at   TEXT,
    completed_at        TEXT,
    UNIQUE (user_id, exercise_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE CASCADE
);

-- ============================================================================
-- INDEXES: Tối ưu hiệu năng truy vấn và kết nối khóa ngoại
-- ============================================================================
CREATE INDEX idx_sessions_user_id ON sessions(user_id);
CREATE INDEX idx_pets_user_id ON pets(user_id);
CREATE INDEX idx_inventory_user_id ON inventory_items(user_id);
CREATE INDEX idx_inventory_item_id ON inventory_items(item_id);
CREATE INDEX idx_test_cases_exercise ON test_cases(exercise_id);
CREATE INDEX idx_submissions_user_ex ON submissions(user_id, exercise_id);
CREATE INDEX idx_progress_user_ex ON progress(user_id, exercise_id);

-- ============================================================================
-- DỮ LIỆU KHỞI TẠO (SEED DATA): Khớp dữ liệu đồ án Pet Raising
-- ============================================================================

-- Dữ liệu tài khoản mẫu
INSERT INTO users (id, username, email, password_hash, coins)
VALUES (1, 'PetLovery', 'petlovery@example.com', 'scrypt:default_hashed_pass', 100);

-- Thú cưng mẫu
INSERT INTO pets (id, user_id, name, type, level, experience, happiness, hunger, energy)
VALUES (1, 1, 'UIA', 'CAT', 1, 0, 85, 30, 70);

-- Danh mục vật phẩm (khớp với catalog trong hệ thống)
INSERT INTO items (id, name, description, category, filter, price, image_path, icon, hunger_restore, happiness_restore, slot) VALUES
(1, 'Bread', 'A filling snack at a friendly price.', 'food', 'food', 5, 'assets/items/banh_mi.png', '🥖', 30, 0, NULL),
(2, 'Apple', 'A crisp and refreshing fruit.', 'food', 'food', 8, 'assets/items/tao_do.png', '🍎', 20, 5, NULL),
(3, 'Golden Apple', 'A rare apple with a generous boost.', 'food', 'food', 25, 'assets/items/tao_vang.png', '🍏', 40, 15, NULL),
(4, 'Cream Cake', 'A soft, sweet treat that lifts the mood.', 'food', 'food', 6, 'assets/items/banh_kem.png', '🍰', 10, 20, NULL),
(5, 'Honey', 'Naturally sweet and nourishing.', 'food', 'food', 3, 'assets/items/mat_ong.png', '🍯', 5, 25, NULL),
(6, 'Wool Hat', 'A warm knitted hat for your pet.', 'decoration', 'headwear', 15, 'assets/items/mu_len.png', '🧢', 0, 0, 'headwear'),
(7, 'Crown', 'A royal crown for your pet.', 'decoration', 'headwear', 50, 'assets/items/vuong_mien.png', '👑', 0, 0, 'headwear'),
(8, 'Beach Escape', 'A sunny shore with clear blue water.', 'decoration', 'background', 20, 'assets/items/nen_bai_bien.png', '🏖️', 0, 0, 'background'),
(9, 'Starry Sky', 'A calm night sky filled with stars.', 'decoration', 'background', 30, 'assets/items/nen_bau_troi.png', '🌌', 0, 0, 'background'),
(10, 'Wooden Floor', 'A warm wooden floor for a cozy room.', 'decoration', 'floor', 18, 'assets/items/san_go.png', '🪵', 0, 0, 'floor'),
(11, 'Flower Rug', 'A soft rug with a cheerful floral pattern.', 'decoration', 'floor', 22, 'assets/items/tham_hoa.png', '🌼', 0, 0, 'floor'),
(12, 'Forest Hideaway', 'A peaceful scene among green trees.', 'decoration', 'background', 25, 'assets/items/nen_khu_rung.png', '🌳', 0, 0, 'background'),
(13, 'City Lights', 'A lively city glowing after dark.', 'decoration', 'background', 35, 'assets/items/nen_thanh_pho.png', '🏙️', 0, 0, 'background');

-- Túi đồ ban đầu của người chơi
INSERT INTO inventory_items (user_id, item_id, quantity, is_equipped) VALUES
(1, 1, 3, 0), -- 3 Bánh mì
(1, 2, 2, 0); -- 2 Táo đỏ

-- Danh sách bài tập C/C++ mẫu
INSERT INTO exercises (id, title, description, difficulty, max_score, reward_coins, reward_exp) VALUES
(1, 'Hello World', 'Return exactly Hello World! (case sensitive).', 'easy', 100, 10, 20),
(2, 'Addition', 'Compute 7 + 5. Return the integer result.', 'easy', 100, 10, 20),
(3, 'Parity', 'Is 14 even or odd? Return exactly even (lowercase).', 'easy', 100, 10, 20),
(4, 'Maximum', 'Find the maximum of 3 9 2 7. Return the integer result.', 'easy', 100, 15, 30),
(5, 'Sorting', 'Sort 5 1 4 2 3 ascending. Return space-separated integers.', 'easy', 100, 20, 40);

-- Bộ Test cases cho các bài tập
INSERT INTO test_cases (exercise_id, input_data, expected_output, is_hidden, ordinal) VALUES
(1, '', 'Hello World!', 0, 1),
(2, '7 5', '12', 0, 1),
(3, '14', 'even', 0, 1),
(3, '15', 'odd', 1, 2),
(4, '3 9 2 7', '9', 0, 1),
(5, '5 1 4 2 3', '1 2 3 4 5', 0, 1);

-- Nhiệm vụ mẫu (Quests)
INSERT INTO quests (id, title, description, quest_type, target_action, target_count, reward_coins, reward_exp, reward_item_id) VALUES
(1, 'Daily Meal', 'Feed your pet 1 time today.', 'DAILY', 'FEED_PET', 1, 20, 15, 2),
(2, 'Daily Study', 'Complete 1 programming exercise.', 'DAILY', 'SOLVE_EXERCISE', 1, 30, 25, NULL),
(3, 'Fashionista', 'Buy 1 decoration item from Little Paws Market.', 'ACHIEVEMENT', 'BUY_ITEM', 1, 50, 40, NULL);
