#include "Database.h"
#include "Catalog.h"
#include "Crypto.h"
#include "Sha256.h"
#include "sqlite3.h"
#include <sstream>
#include <stdexcept>

namespace {
class Statement {
    sqlite3_stmt* statement_ = nullptr;
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(database));
    }
    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    void bind(int index, long long value) {
        if (sqlite3_bind_int64(statement_, index, value) != SQLITE_OK) throw std::runtime_error("SQL bind failed");
    }
    void bind(int index, const std::string& value) {
        if (sqlite3_bind_text(statement_, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK)
            throw std::runtime_error("SQL bind failed");
    }
    bool row() {
        int result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) return true;
        if (result == SQLITE_DONE) return false;
        throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(statement_)));
    }
    void done() { if (row()) throw std::runtime_error("Unexpected SQL row"); }
    long long number(int column) { return sqlite3_column_int64(statement_, column); }
    std::string text(int column) {
        const auto* value = sqlite3_column_text(statement_, column);
        return value ? std::string(reinterpret_cast<const char*>(value), sqlite3_column_bytes(statement_, column)) : "";
    }
};

void execute(sqlite3* database, const char* sql) {
    if (sqlite3_exec(database, sql, nullptr, nullptr, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(database));
}

class Transaction {
    sqlite3* database_;
    bool committed_ = false;
public:
    explicit Transaction(sqlite3* database) : database_(database) { execute(database_, "BEGIN IMMEDIATE;"); }
    ~Transaction() { if (!committed_) sqlite3_exec(database_, "ROLLBACK;", nullptr, nullptr, nullptr); }
    void commit() { execute(database_, "COMMIT;"); committed_ = true; }
};

bool hasColumn(sqlite3* database, const char* table, const char* column) {
    Statement statement(database, (std::string("PRAGMA table_info(") + table + ");").c_str());
    while (statement.row()) if (statement.text(1) == column) return true;
    return false;
}

ProgressRow progressRow(Statement& statement) {
    ProgressRow progress;
    progress.exerciseId = statement.number(0);
    progress.status = statement.text(1);
    progress.bestScore = static_cast<int>(statement.number(2));
    progress.attempts = static_cast<int>(statement.number(3));
    progress.completedAt = statement.text(4);
    return progress;
}

std::string normalizedAnswer(const std::string& answer) {
    std::istringstream input(answer);
    std::string word, result;
    while (input >> word) { if (!result.empty()) result += ' '; result += word; }
    return result;
}
}

Database::Database() {}
Database::~Database() { close(); }

bool Database::open(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        lastError_ = db_ ? sqlite3_errmsg(db_) : "Cannot open database";
        return false;
    }
    sqlite3_busy_timeout(db_, 3000);
    return exec("PRAGMA foreign_keys=ON; PRAGMA journal_mode=WAL;");
}

void Database::close() { if (db_) { sqlite3_close(db_); db_ = nullptr; } }

bool Database::exec(const std::string& sql) {
    try { execute(db_, sql.c_str()); return true; }
    catch (const std::exception& error) { lastError_ = error.what(); return false; }
}

bool Database::runSchema() {
    try {
        Transaction transaction(db_);
        execute(db_,
            "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT,email TEXT NOT NULL UNIQUE,"
            "password_salt TEXT NOT NULL,password_hash TEXT NOT NULL,coins INTEGER NOT NULL DEFAULT 100,"
            "created_at TEXT NOT NULL DEFAULT (datetime('now')),last_login_at TEXT);"
            "CREATE TABLE IF NOT EXISTS pets (user_id INTEGER PRIMARY KEY REFERENCES users(id),name TEXT NOT NULL DEFAULT 'UIA',"
            "level INTEGER NOT NULL DEFAULT 1,experience INTEGER NOT NULL DEFAULT 0,happiness INTEGER NOT NULL DEFAULT 85,"
            "hunger INTEGER NOT NULL DEFAULT 30,energy INTEGER NOT NULL DEFAULT 70,species TEXT NOT NULL DEFAULT 'cat');"
            "CREATE TABLE IF NOT EXISTS sessions (token TEXT PRIMARY KEY,user_id INTEGER NOT NULL REFERENCES users(id),"
            "created_at TEXT NOT NULL DEFAULT (datetime('now')),expires_at INTEGER NOT NULL);"
            "CREATE TABLE IF NOT EXISTS exercises (id INTEGER PRIMARY KEY AUTOINCREMENT,title TEXT NOT NULL,"
            "description TEXT NOT NULL DEFAULT '',difficulty TEXT NOT NULL DEFAULT 'easy',max_score INTEGER NOT NULL DEFAULT 100);"
            "CREATE TABLE IF NOT EXISTS submissions (id INTEGER PRIMARY KEY AUTOINCREMENT,user_id INTEGER NOT NULL REFERENCES users(id),"
            "exercise_id INTEGER NOT NULL REFERENCES exercises(id),score INTEGER NOT NULL DEFAULT 0,status TEXT NOT NULL DEFAULT 'submitted',"
            "submitted_at TEXT NOT NULL DEFAULT (datetime('now')),answer TEXT NOT NULL DEFAULT '');"
            "CREATE TABLE IF NOT EXISTS user_exercise_progress (user_id INTEGER NOT NULL,exercise_id INTEGER NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'in_progress',best_score INTEGER NOT NULL DEFAULT 0,attempts INTEGER NOT NULL DEFAULT 0,"
            "completed_at TEXT,PRIMARY KEY(user_id,exercise_id));"
            "CREATE TABLE IF NOT EXISTS inventory (user_id INTEGER NOT NULL REFERENCES users(id),item_id INTEGER NOT NULL,"
            "quantity INTEGER NOT NULL CHECK(quantity BETWEEN 1 AND 1000000),PRIMARY KEY(user_id,item_id),"
            "CHECK(item_id BETWEEN 1 AND 13),CHECK(item_id BETWEEN 1 AND 5 OR quantity=1));"
            "CREATE TABLE IF NOT EXISTS purchase_transactions (id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER NOT NULL REFERENCES users(id),item_id INTEGER NOT NULL,quantity INTEGER NOT NULL CHECK(quantity>0),"
            "unit_price INTEGER NOT NULL CHECK(unit_price>0),total INTEGER NOT NULL CHECK(total=quantity*unit_price),"
            "created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')));"
        );
        if (!hasColumn(db_, "sessions", "expires_at"))
            execute(db_, "ALTER TABLE sessions ADD COLUMN expires_at INTEGER NOT NULL DEFAULT 0;");
        if (!hasColumn(db_, "submissions", "answer"))
            execute(db_, "ALTER TABLE submissions ADD COLUMN answer TEXT NOT NULL DEFAULT '';");
        if (!hasColumn(db_, "pets", "species"))
            execute(db_, "ALTER TABLE pets ADD COLUMN species TEXT NOT NULL DEFAULT 'cat';");
        execute(db_, "DELETE FROM sessions WHERE expires_at<=CAST(strftime('%s','now') AS INTEGER);");
        transaction.commit();
        return true;
    } catch (const std::exception& error) { lastError_ = error.what(); return false; }
}

bool Database::seedExercisesIfEmpty() {
    return exec(
        "BEGIN IMMEDIATE;"
        "INSERT OR IGNORE INTO exercises(id,title) VALUES (1,'Hello World'),(2,'Addition'),(3,'Parity'),(4,'Maximum'),(5,'Sorting');"
        "UPDATE exercises SET description=CASE id "
        "WHEN 1 THEN 'Return exactly Hello World! (case sensitive).' "
        "WHEN 2 THEN 'Compute 7 + 5. Return the integer result.' "
        "WHEN 3 THEN 'Is 14 even or odd? Return exactly even (lowercase).' "
        "WHEN 4 THEN 'Find the maximum of 3 9 2 7. Return the integer result.' "
        "WHEN 5 THEN 'Sort 5 1 4 2 3 ascending. Return space-separated integers.' END,"
        "max_score=100 WHERE id BETWEEN 1 AND 5; COMMIT;"
    );
}

bool Database::emailExists(const std::string& email) {
    Statement statement(db_, "SELECT 1 FROM users WHERE email=?;"); statement.bind(1,email); return statement.row();
}

bool Database::createUser(const std::string& email, const std::string& password, UserRow& user) {
    std::string salt = crypto::randomHex(16);
    std::string hash = crypto::passwordHash(password, salt);
    Transaction transaction(db_);
    Statement statement(db_, "INSERT INTO users(email,password_salt,password_hash) VALUES(?,?,?);");
    statement.bind(1,email); statement.bind(2,salt); statement.bind(3,hash); statement.done();
    user.id = sqlite3_last_insert_rowid(db_); user.email = email; user.coins = 100;
    Statement pet(db_, "INSERT INTO pets(user_id) VALUES(?);"); pet.bind(1,user.id); pet.done();
    transaction.commit();
    return true;
}

bool Database::verifyLogin(const std::string& email, const std::string& password, UserRow& user) {
    std::string salt, hash;
    {
        Statement statement(db_, "SELECT id,coins,password_salt,password_hash FROM users WHERE email=?;");
        statement.bind(1,email);
        if (statement.row()) {
            user.id = statement.number(0); user.coins = static_cast<int>(statement.number(1)); user.email = email;
            salt = statement.text(2); hash = statement.text(3);
        }
    }
    if (hash.empty()) {
        crypto::passwordHash(password, "00000000000000000000000000000000");
        return false;
    }
    if (hash.compare(0, 21, "pbkdf2-sha256$600000$") == 0)
        return crypto::constantEqual(crypto::passwordHash(password,salt), hash);
    if (hash.size() != 64 || !crypto::constantEqual(sha256::hashHex(salt + password), hash)) return false;
    std::string newSalt = crypto::randomHex(16);
    std::string newHash = crypto::passwordHash(password,newSalt);
    Statement migrate(db_, "UPDATE users SET password_salt=?,password_hash=? WHERE id=?;");
    migrate.bind(1,newSalt); migrate.bind(2,newHash); migrate.bind(3,user.id); migrate.done();
    return true;
}

bool Database::touchLastLogin(long long userId) {
    Statement statement(db_, "UPDATE users SET last_login_at=datetime('now') WHERE id=?;");
    statement.bind(1,userId); statement.done(); return true;
}

std::string Database::createSession(long long userId) {
    execute(db_, "DELETE FROM sessions WHERE expires_at<=CAST(strftime('%s','now') AS INTEGER);");
    std::string token = crypto::randomHex(32);
    Statement statement(db_, "INSERT INTO sessions(token,user_id,expires_at) VALUES(?,?,CAST(strftime('%s','now') AS INTEGER)+604800);");
    statement.bind(1,sha256::hashHex(token)); statement.bind(2,userId); statement.done(); return token;
}

bool Database::userIdForToken(const std::string& token, long long& userId) {
    if (token.size() != 64 || token.find_first_not_of("0123456789abcdef") != std::string::npos) return false;
    Statement statement(db_, "SELECT user_id FROM sessions WHERE token=? AND expires_at>CAST(strftime('%s','now') AS INTEGER);");
    statement.bind(1,sha256::hashHex(token));
    if (!statement.row()) return false;
    userId = statement.number(0); return true;
}

bool Database::revokeSession(const std::string& token) {
    Statement statement(db_, "DELETE FROM sessions WHERE token=?;");
    statement.bind(1,sha256::hashHex(token)); statement.done(); return true;
}

bool Database::getUserById(long long userId, UserRow& user) {
    Statement statement(db_, "SELECT id,email,coins FROM users WHERE id=?;"); statement.bind(1,userId);
    if (!statement.row()) return false;
    user.id = statement.number(0); user.email = statement.text(1); user.coins = static_cast<int>(statement.number(2)); return true;
}

bool Database::getPet(long long userId, PetRow& pet) {
    Statement statement(db_, "SELECT user_id,name,level,experience,happiness,hunger,energy,species FROM pets WHERE user_id=?;");
    statement.bind(1,userId); if (!statement.row()) return false;
    pet.userId = statement.number(0); pet.name = statement.text(1); pet.level = static_cast<int>(statement.number(2));
    pet.experience = static_cast<int>(statement.number(3)); pet.happiness = static_cast<int>(statement.number(4));
    pet.hunger = static_cast<int>(statement.number(5)); pet.energy = static_cast<int>(statement.number(6));
    pet.species = statement.text(7); return true;
}

bool Database::renamePet(long long userId, const std::string& name) {
    Statement statement(db_, "UPDATE pets SET name=? WHERE user_id=?;");
    statement.bind(1,name); statement.bind(2,userId); statement.done(); return sqlite3_changes(db_) == 1;
}

bool Database::setPetSpecies(long long userId, const std::string& species) {
    if (species != "cat" && species != "dog") return false;
    Statement statement(db_, "UPDATE pets SET species=? WHERE user_id=?;");
    statement.bind(1,species); statement.bind(2,userId); statement.done(); return sqlite3_changes(db_) == 1;
}

std::vector<InventoryRow> Database::listInventory(long long userId) {
    std::vector<InventoryRow> rows;
    Statement statement(db_, "SELECT item_id,quantity FROM inventory WHERE user_id=? ORDER BY item_id;"); statement.bind(1,userId);
    while (statement.row()) rows.push_back({static_cast<int>(statement.number(0)),static_cast<int>(statement.number(1))});
    return rows;
}

int Database::purchase(long long userId, int itemId, int quantity) {
    const auto* item = shopItem(itemId);
    if (!item) return 404;
    if (quantity < 1 || quantity > 1000 || ((*item)["category"] == "decoration" && quantity != 1)) return 400;
    long long price = (*item)["price"].get<int>();
    long long total = price * quantity;
    Transaction transaction(db_);
    {
        Statement owned(db_, "SELECT quantity FROM inventory WHERE user_id=? AND item_id=?;");
        owned.bind(1,userId); owned.bind(2,itemId);
        if (owned.row()) {
            if ((*item)["category"] == "decoration") return 409;
            if (owned.number(0) + quantity > 1000000) return 409;
        }
    }
    Statement debit(db_, "UPDATE users SET coins=coins-? WHERE id=? AND coins>=?;");
    debit.bind(1,total); debit.bind(2,userId); debit.bind(3,total); debit.done();
    if (sqlite3_changes(db_) != 1) return 409;
    Statement inventory(db_, "INSERT INTO inventory(user_id,item_id,quantity) VALUES(?,?,?) "
        "ON CONFLICT(user_id,item_id) DO UPDATE SET quantity=quantity+excluded.quantity;");
    inventory.bind(1,userId); inventory.bind(2,itemId); inventory.bind(3,quantity); inventory.done();
    Statement ledger(db_, "INSERT INTO purchase_transactions(user_id,item_id,quantity,unit_price,total) VALUES(?,?,?,?,?);");
    ledger.bind(1,userId); ledger.bind(2,itemId); ledger.bind(3,quantity); ledger.bind(4,price); ledger.bind(5,total); ledger.done();
    transaction.commit(); return 200;
}

int Database::feed(long long userId, int itemId) {
    const auto* item = shopItem(itemId);
    if (!item) return 404;
    if ((*item)["category"] != "food") return 400;
    Transaction transaction(db_);
    long long quantity = 0;
    {
        Statement owned(db_, "SELECT quantity FROM inventory WHERE user_id=? AND item_id=?;");
        owned.bind(1,userId); owned.bind(2,itemId);
        if (owned.row()) quantity = owned.number(0);
    }
    if (quantity < 1) return 409;
    Statement consume(db_, quantity == 1
        ? "DELETE FROM inventory WHERE user_id=? AND item_id=?;"
        : "UPDATE inventory SET quantity=quantity-1 WHERE user_id=? AND item_id=?;");
    consume.bind(1,userId); consume.bind(2,itemId); consume.done();
    Statement pet(db_, "UPDATE pets SET hunger=MAX(0,MIN(100,hunger+?)),"
        "happiness=MAX(0,MIN(100,happiness+?)) WHERE user_id=?;");
    pet.bind(1,(*item)["hungerRestore"].get<int>());
    pet.bind(2,(*item)["happinessRestore"].get<int>());
    pet.bind(3,userId); pet.done();
    if (sqlite3_changes(db_) != 1) return 404;
    transaction.commit(); return 200;
}

std::vector<ExerciseRow> Database::listExercises() {
    std::vector<ExerciseRow> rows;
    Statement statement(db_, "SELECT id,title,description,difficulty,max_score FROM exercises WHERE id BETWEEN 1 AND 5 ORDER BY id;");
    while (statement.row()) {
        ExerciseRow row; row.id = statement.number(0); row.title = statement.text(1); row.description = statement.text(2);
        row.difficulty = statement.text(3); row.maxScore = static_cast<int>(statement.number(4)); rows.push_back(row);
    }
    return rows;
}

bool Database::exerciseExists(long long exerciseId) {
    if (exerciseId < 1 || exerciseId > 5) return false;
    Statement statement(db_, "SELECT 1 FROM exercises WHERE id=?;"); statement.bind(1,exerciseId); return statement.row();
}

std::vector<ProgressRow> Database::listProgress(long long userId) {
    std::vector<ProgressRow> rows;
    Statement statement(db_, "SELECT exercise_id,status,best_score,attempts,IFNULL(completed_at,'') FROM user_exercise_progress WHERE user_id=? ORDER BY exercise_id;");
    statement.bind(1,userId); while (statement.row()) rows.push_back(progressRow(statement)); return rows;
}

bool Database::recordSubmission(long long userId, long long exerciseId, const std::string& answer, int& score, ProgressRow& progress) {
    static const char* answers[] = {"Hello World!", "12", "even", "9", "1 2 3 4 5"};
    if (!exerciseExists(exerciseId)) return false;
    score = normalizedAnswer(answer) == answers[exerciseId-1] ? 100 : 0;
    std::string status = score == 100 ? "completed" : "in_progress";
    Transaction transaction(db_);
    Statement submission(db_, "INSERT INTO submissions(user_id,exercise_id,answer,score,status) VALUES(?,?,?,?,?);");
    submission.bind(1,userId); submission.bind(2,exerciseId); submission.bind(3,answer); submission.bind(4,score);
    submission.bind(5,status); submission.done();
    Statement update(db_,
        "INSERT INTO user_exercise_progress(user_id,exercise_id,status,best_score,attempts,completed_at) "
        "VALUES(?,?,?,?,1,CASE WHEN ?=100 THEN strftime('%Y-%m-%dT%H:%M:%fZ','now') END) "
        "ON CONFLICT(user_id,exercise_id) DO UPDATE SET attempts=attempts+1,best_score=MAX(best_score,excluded.best_score),"
        "status=CASE WHEN best_score=100 OR status='completed' OR excluded.best_score=100 THEN 'completed' ELSE 'in_progress' END,"
        "completed_at=COALESCE(completed_at,excluded.completed_at);");
    update.bind(1,userId); update.bind(2,exerciseId); update.bind(3,status); update.bind(4,score); update.bind(5,score); update.done();
    Statement select(db_, "SELECT exercise_id,status,best_score,attempts,IFNULL(completed_at,'') FROM user_exercise_progress WHERE user_id=? AND exercise_id=?;");
    select.bind(1,userId); select.bind(2,exerciseId); if (!select.row()) return false;
    progress = progressRow(select);
    transaction.commit(); return true;
}
