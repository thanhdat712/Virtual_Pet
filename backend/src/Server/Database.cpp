#include "Database.h"
#include "Exercise.h"
#include "Submission.h"
#include "Catalog.h"
#include "Crypto.h"
#include "Pet.h"
#include "Sha256.h"
#include "sqlite3.h"
#include <fstream>
#include <iterator>
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

bool ensureStudyState(sqlite3* database, long long userId) {
    Statement user(database, "SELECT 1 FROM users WHERE id=?;");
    user.bind(1,userId);
    if (!user.row()) return false;
    Statement statement(database, "INSERT OR IGNORE INTO study_state(user_id) SELECT id FROM users WHERE id=?;");
    statement.bind(1,userId);
    statement.done();
    return true;
}

bool petNeedsStudyCare(const StudyStateRow& state) {
    return state.hunger < 10 || state.energy < 10;
}

bool loadStudyState(sqlite3* database, long long userId, StudyStateRow& state) {
    Statement statement(database,
        "SELECT s.total_points,u.coins,p.level,p.happiness,p.hunger,p.energy "
        "FROM study_state s JOIN users u ON u.id=s.user_id "
        "JOIN pets p ON p.user_id=s.user_id WHERE s.user_id=?;");
    statement.bind(1,userId);
    if (!statement.row()) return false;
    state.points = statement.number(0);
    state.coins = static_cast<int>(statement.number(1));
    state.level = static_cast<int>(statement.number(2));
    state.happiness = static_cast<int>(statement.number(3));
    state.hunger = static_cast<int>(statement.number(4));
    state.energy = static_cast<int>(statement.number(5));

    state.completedStages.clear();
    Statement completed(database, "SELECT stage FROM study_stage_progress WHERE user_id=? ORDER BY stage_order;");
    completed.bind(1,userId);
    while (completed.row()) state.completedStages.push_back(completed.text(0));
    return true;
}

bool ensurePracticeState(sqlite3* database, long long userId) {
    Statement user(database, "SELECT 1 FROM users WHERE id=?;");
    user.bind(1,userId);
    if (!user.row()) return false;
    Statement statement(database, "INSERT OR IGNORE INTO practice_state(user_id) SELECT id FROM users WHERE id=?;");
    statement.bind(1,userId);
    statement.done();
    return true;
}

bool loadPracticeState(sqlite3* database, long long userId, PracticeStateRow& state) {
    Statement statement(database,
        "SELECT s.total_points,s.best_score,u.coins,p.level,p.happiness,p.hunger,p.energy "
        "FROM practice_state s JOIN users u ON u.id=s.user_id "
        "JOIN pets p ON p.user_id=s.user_id WHERE s.user_id=?;");
    statement.bind(1,userId);
    if (!statement.row()) return false;
    state.points = statement.number(0);
    state.bestScore = static_cast<int>(statement.number(1));
    state.coins = static_cast<int>(statement.number(2));
    state.level = static_cast<int>(statement.number(3));
    state.happiness = static_cast<int>(statement.number(4));
    state.hunger = static_cast<int>(statement.number(5));
    state.energy = static_cast<int>(statement.number(6));
    return true;
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
        std::ifstream schemaFile("../../database/schema.sql",std::ios::binary);
        if (!schemaFile) {
            schemaFile.clear();
            schemaFile.open("database/schema.sql",std::ios::binary);
        }
        if (!schemaFile) throw std::runtime_error("Cannot open database/schema.sql");
        const std::string schema((std::istreambuf_iterator<char>(schemaFile)),std::istreambuf_iterator<char>());
        Transaction transaction(db_);
        execute(db_,schema.c_str());
        if (!hasColumn(db_, "users", "username"))
            execute(db_, "ALTER TABLE users ADD COLUMN username TEXT;");
        if (!hasColumn(db_, "users", "sound_enabled"))
            execute(db_, "ALTER TABLE users ADD COLUMN sound_enabled INTEGER NOT NULL DEFAULT 1;");
        if (!hasColumn(db_, "users", "email_verified"))
            execute(db_, "ALTER TABLE users ADD COLUMN email_verified INTEGER NOT NULL DEFAULT 1;");
        if (!hasColumn(db_, "pets", "species"))
            execute(db_, "ALTER TABLE pets ADD COLUMN species TEXT NOT NULL DEFAULT 'cat';");
        if (!hasColumn(db_, "pets", "species_chosen")) {
            execute(db_, "ALTER TABLE pets ADD COLUMN species_chosen INTEGER NOT NULL DEFAULT 0;");
            execute(db_, "UPDATE pets SET species_chosen=1;");
        }
        execute(db_, "UPDATE users SET username='player_'||id WHERE username IS NULL OR username='';"
            "CREATE UNIQUE INDEX IF NOT EXISTS users_username_unique ON users(username COLLATE NOCASE);");
        if (!hasColumn(db_, "sessions", "expires_at"))
            execute(db_, "ALTER TABLE sessions ADD COLUMN expires_at INTEGER NOT NULL DEFAULT 0;");
        if (!hasColumn(db_, "submissions", "answer"))
            execute(db_, "ALTER TABLE submissions ADD COLUMN answer TEXT NOT NULL DEFAULT '';");
        execute(db_, "CREATE TABLE IF NOT EXISTS code_submissions ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,"
            "source_code TEXT NOT NULL, language TEXT NOT NULL DEFAULT 'cpp',"
            "status TEXT NOT NULL DEFAULT 'queued', score INTEGER NOT NULL DEFAULT 0,"
            "message TEXT NOT NULL DEFAULT '', submitted_at TEXT NOT NULL, finished_at TEXT NOT NULL DEFAULT '');"
            "CREATE INDEX IF NOT EXISTS code_submissions_user_idx ON code_submissions(user_id, id DESC);"
            "CREATE TABLE IF NOT EXISTS code_submission_rewards ("
            "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,"
            "submission_id INTEGER NOT NULL REFERENCES code_submissions(id) ON DELETE CASCADE,"
            "coins_awarded INTEGER NOT NULL DEFAULT 10 CHECK(coins_awarded>=0),"
            "awarded_at TEXT NOT NULL, PRIMARY KEY(user_id,exercise_id));"
            "CREATE INDEX IF NOT EXISTS code_submissions_queue_idx ON code_submissions(status,id);");
        execute(db_, "CREATE TABLE IF NOT EXISTS password_reset_tokens ("
            "token_hash TEXT PRIMARY KEY,"
            "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "expires_at INTEGER NOT NULL);");
        execute(db_, "CREATE TABLE IF NOT EXISTS email_verification_tokens ("
            "token_hash TEXT PRIMARY KEY,"
            "user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "expires_at INTEGER NOT NULL);");
        execute(db_, "CREATE TABLE IF NOT EXISTS pending_email_changes ("
            "user_id INTEGER PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,"
            "email TEXT NOT NULL UNIQUE COLLATE NOCASE,"
            "token_hash TEXT NOT NULL UNIQUE,"
            "expires_at INTEGER NOT NULL);");
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

bool Database::issueEmailVerification(const std::string& email, const std::string& tokenHash, long long expiresAt) {
    Transaction transaction(db_);
    Statement user(db_, "SELECT id,email_verified FROM users WHERE email=? COLLATE NOCASE;");
    user.bind(1,email);
    if (!user.row() || user.number(1) != 0) { transaction.commit(); return false; }
    const long long userId = user.number(0);
    Statement removeOld(db_, "DELETE FROM email_verification_tokens WHERE user_id=?;");
    removeOld.bind(1,userId); removeOld.done();
    Statement insert(db_, "INSERT INTO email_verification_tokens(token_hash,user_id,expires_at) VALUES(?,?,?);");
    insert.bind(1,tokenHash); insert.bind(2,userId); insert.bind(3,expiresAt); insert.done();
    transaction.commit();
    return true;
}

bool Database::verifyEmail(const std::string& email, const std::string& tokenHash) {
    Transaction transaction(db_);
    Statement token(db_, "SELECT user_id FROM email_verification_tokens WHERE token_hash=? AND expires_at>CAST(strftime('%s','now') AS INTEGER);");
    token.bind(1,tokenHash);
    if (!token.row()) { transaction.commit(); return false; }
    const long long userId = token.number(0);
    Statement user(db_, "UPDATE users SET email_verified=1 WHERE id=? AND email=? COLLATE NOCASE AND email_verified=0;");
    user.bind(1,userId); user.bind(2,email); user.done();
    if (sqlite3_changes(db_) != 1) return false;
    Statement removeTokens(db_, "DELETE FROM email_verification_tokens WHERE user_id=?;");
    removeTokens.bind(1,userId); removeTokens.done();
    transaction.commit();
    return true;
}

bool Database::createPendingEmailChange(long long userId, const std::string& email,
                                       const std::string& tokenHash, long long expiresAt) {
    Transaction transaction(db_);
    Statement existing(db_, "SELECT id FROM users WHERE email=? COLLATE NOCASE AND id<>?;");
    existing.bind(1,email); existing.bind(2,userId);
    if (existing.row()) { transaction.commit(); return false; }
    Statement existingPending(db_, "SELECT user_id FROM pending_email_changes WHERE email=? COLLATE NOCASE AND user_id<>?;");
    existingPending.bind(1,email); existingPending.bind(2,userId);
    if (existingPending.row()) { transaction.commit(); return false; }
    Statement removeOld(db_, "DELETE FROM pending_email_changes WHERE user_id=?;");
    removeOld.bind(1,userId); removeOld.done();
    Statement insert(db_, "INSERT INTO pending_email_changes(user_id,email,token_hash,expires_at) VALUES(?,?,?,?);");
    insert.bind(1,userId); insert.bind(2,email); insert.bind(3,tokenHash); insert.bind(4,expiresAt); insert.done();
    transaction.commit();
    return true;
}

bool Database::confirmEmailChange(long long userId, const std::string& email, const std::string& tokenHash) {
    Transaction transaction(db_);
    Statement pending(db_, "SELECT 1 FROM pending_email_changes WHERE user_id=? AND email=? COLLATE NOCASE AND token_hash=? AND expires_at>CAST(strftime('%s','now') AS INTEGER);");
    pending.bind(1,userId); pending.bind(2,email); pending.bind(3,tokenHash);
    if (!pending.row()) { transaction.commit(); return false; }
    Statement update(db_, "UPDATE users SET email=?,email_verified=1 WHERE id=?;");
    update.bind(1,email); update.bind(2,userId); update.done();
    if (sqlite3_changes(db_) != 1) return false;
    Statement remove(db_, "DELETE FROM pending_email_changes WHERE user_id=?;");
    remove.bind(1,userId); remove.done();
    transaction.commit();
    return true;
}

bool Database::usernameExists(const std::string& username) {
    Statement statement(db_, "SELECT 1 FROM users WHERE username=? COLLATE NOCASE;");
    statement.bind(1,username); return statement.row();
}

bool Database::createUser(const std::string& email, const std::string& password, UserRow& user) {
    return createUser("",email,password,user);
}

bool Database::createUser(const std::string& username, const std::string& email,
                          const std::string& password, UserRow& user) {
    std::string salt = crypto::randomHex(16);
    std::string hash = crypto::passwordHash(password, salt);
    Transaction transaction(db_);
    Statement statement(db_, "INSERT INTO users(username,email,password_salt,password_hash,email_verified) VALUES(?,?,?,?,1);");
    statement.bind(1,username.empty() ? "pending_"+crypto::randomHex(8) : username);
    statement.bind(2,email); statement.bind(3,salt); statement.bind(4,hash); statement.done();
    user.id = sqlite3_last_insert_rowid(db_);
    user.username = username.empty() ? "player_"+std::to_string(user.id) : username;
    user.email = email; user.coins = 100; user.soundEnabled = true;
    Statement updateUser(db_, "UPDATE users SET username=? WHERE id=?;");
    updateUser.bind(1,user.username); updateUser.bind(2,user.id); updateUser.done();
    Statement pet(db_, "INSERT INTO pets(user_id) VALUES(?);"); pet.bind(1,user.id); pet.done();
    transaction.commit();
    return true;
}

bool Database::verifyLogin(const std::string& email, const std::string& password, UserRow& user) {
    std::string salt, hash;
    {
        Statement statement(db_, "SELECT id,username,coins,sound_enabled,password_salt,password_hash,email_verified FROM users WHERE email=?;");
        statement.bind(1,email);
        if (statement.row()) {
            user.id = statement.number(0); user.username = statement.text(1);
            user.coins = static_cast<int>(statement.number(2)); user.soundEnabled = statement.number(3) != 0;
            user.email = email; salt = statement.text(4); hash = statement.text(5);
            user.emailVerified = statement.number(6) != 0;
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

bool Database::createPasswordReset(const std::string& email, const std::string& tokenHash, long long expiresAt) {
    Transaction transaction(db_);
    Statement user(db_, "SELECT id FROM users WHERE email=? COLLATE NOCASE;");
    user.bind(1,email);
    if (!user.row()) { transaction.commit(); return false; }
    const long long userId = user.number(0);
    Statement removeOld(db_, "DELETE FROM password_reset_tokens WHERE user_id=?;");
    removeOld.bind(1,userId); removeOld.done();
    Statement insert(db_, "INSERT INTO password_reset_tokens(token_hash,user_id,expires_at) VALUES(?,?,?);");
    insert.bind(1,tokenHash); insert.bind(2,userId); insert.bind(3,expiresAt); insert.done();
    transaction.commit();
    return true;
}

bool Database::resetPassword(const std::string& tokenHash, const std::string& salt, const std::string& passwordHash) {
    Transaction transaction(db_);
    Statement token(db_, "SELECT user_id FROM password_reset_tokens WHERE token_hash=? AND expires_at>CAST(strftime('%s','now') AS INTEGER);");
    token.bind(1,tokenHash);
    if (!token.row()) { transaction.commit(); return false; }
    const long long userId = token.number(0);
    Statement update(db_, "UPDATE users SET password_salt=?,password_hash=? WHERE id=?;");
    update.bind(1,salt); update.bind(2,passwordHash); update.bind(3,userId); update.done();
    if (sqlite3_changes(db_) != 1) return false;
    Statement removeTokens(db_, "DELETE FROM password_reset_tokens WHERE user_id=?;");
    removeTokens.bind(1,userId); removeTokens.done();
    Statement revokeSessions(db_, "DELETE FROM sessions WHERE user_id=?;");
    revokeSessions.bind(1,userId); revokeSessions.done();
    transaction.commit();
    return true;
}

bool Database::changePassword(long long userId, const std::string& currentPassword,
                              const std::string& newPassword, const std::string& currentSessionHash) {
    UserRow account, verifiedAccount;
    if (!getUserById(userId,account) || !verifyLogin(account.email,currentPassword,verifiedAccount) ||
        verifiedAccount.id != userId) return false;

    const std::string salt = crypto::randomHex(16);
    const std::string hash = crypto::passwordHash(newPassword,salt);
    Transaction transaction(db_);
    Statement update(db_, "UPDATE users SET password_salt=?,password_hash=? WHERE id=?;");
    update.bind(1,salt); update.bind(2,hash); update.bind(3,userId); update.done();
    if (sqlite3_changes(db_) != 1) return false;
    Statement revokeOtherSessions(db_, "DELETE FROM sessions WHERE user_id=? AND token<>?;");
    revokeOtherSessions.bind(1,userId); revokeOtherSessions.bind(2,currentSessionHash); revokeOtherSessions.done();
    transaction.commit();
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
    Statement statement(db_, "SELECT id,username,email,coins,sound_enabled,email_verified FROM users WHERE id=?;"); statement.bind(1,userId);
    if (!statement.row()) return false;
    user.id = statement.number(0); user.username = statement.text(1); user.email = statement.text(2);
    user.coins = static_cast<int>(statement.number(3)); user.soundEnabled = statement.number(4) != 0;
    user.emailVerified = statement.number(5) != 0; return true;
}

bool Database::setSoundEnabled(long long userId, bool enabled) {
    Statement statement(db_, "UPDATE users SET sound_enabled=? WHERE id=?;");
    statement.bind(1,enabled ? 1 : 0); statement.bind(2,userId); statement.done();
    return sqlite3_changes(db_) == 1;
}

bool Database::getPet(long long userId, PetRow& pet) {
    Statement statement(db_, "SELECT user_id,name,species,species_chosen,level,experience,happiness,hunger,energy FROM pets WHERE user_id=?;");
    statement.bind(1,userId); if (!statement.row()) return false;
    pet.userId = statement.number(0); pet.name = statement.text(1); pet.species = statement.text(2);
    pet.speciesChosen = statement.number(3) != 0;
    pet.level = static_cast<int>(statement.number(4)); pet.experience = static_cast<int>(statement.number(5));
    pet.happiness = static_cast<int>(statement.number(6)); pet.hunger = static_cast<int>(statement.number(7));
    pet.energy = static_cast<int>(statement.number(8)); return true;
}

bool Database::renamePet(long long userId, const std::string& name) {
    Statement statement(db_, "UPDATE pets SET name=? WHERE user_id=?;");
    statement.bind(1,name); statement.bind(2,userId); statement.done(); return sqlite3_changes(db_) == 1;
}

bool Database::savePetProfile(long long userId, const std::string& name, const std::string& species) {
    if (name.empty() || (species != "cat" && species != "dog")) return false;
    Statement statement(db_, "UPDATE pets SET name=?,species=?,species_chosen=1 WHERE user_id=?;");
    statement.bind(1,name); statement.bind(2,species); statement.bind(3,userId); statement.done();
    return sqlite3_changes(db_) == 1;
}

std::vector<InventoryRow> Database::listInventory(long long userId) {
    std::vector<InventoryRow> rows;
    Statement statement(db_, "SELECT item_id,quantity FROM inventory WHERE user_id=? ORDER BY item_id;"); statement.bind(1,userId);
    while (statement.row()) rows.push_back({static_cast<int>(statement.number(0)),static_cast<int>(statement.number(1))});
    return rows;
}

std::vector<EquipmentRow> Database::listEquipment(long long userId) {
    std::vector<EquipmentRow> rows;
    Statement statement(db_, "SELECT slot,item_id FROM equipped_decorations WHERE user_id=? ORDER BY slot;");
    statement.bind(1,userId);
    while (statement.row()) rows.push_back({statement.text(0),static_cast<int>(statement.number(1))});
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
        "level=level+((happiness+?)/100),happiness=((happiness+?)%100) WHERE user_id=?;");
    pet.bind(1,(*item)["hungerRestore"].get<int>());
    pet.bind(2,(*item)["happinessRestore"].get<int>());
    pet.bind(3,(*item)["happinessRestore"].get<int>());
    pet.bind(4,userId); pet.done();
    if (sqlite3_changes(db_) != 1) return 404;
    transaction.commit(); return 200;
}

int Database::performPetAction(long long userId, const std::string& action, int& levelsGained) {
    Transaction transaction(db_);
    int happiness = 0, hunger = 0, energy = 0;
    {
        Statement pet(db_, "SELECT level,happiness,hunger,energy FROM pets WHERE user_id=?;");
        pet.bind(1,userId);
        if (!pet.row()) return 404;
        happiness = static_cast<int>(pet.number(1)); hunger = static_cast<int>(pet.number(2));
        energy = static_cast<int>(pet.number(3));
    }
    int happinessGain = 0;
    if (action == "play") {
        if (energy < Pet::PLAY_ENERGY_COST) return 409;
        energy -= Pet::PLAY_ENERGY_COST;
        hunger = std::max(0,hunger-Pet::PLAY_HUNGER_COST);
        happinessGain = Pet::PLAY_HAPPINESS_GAIN;
    } else if (action == "sleep") {
        if (energy >= Pet::MAX_STAT) return 409;
        energy = std::min(Pet::MAX_STAT,energy+Pet::SLEEP_ENERGY_GAIN);
        hunger = std::max(0,hunger-Pet::SLEEP_HUNGER_COST);
    } else if (action == "bath") {
        energy = std::max(0,energy-Pet::BATH_ENERGY_COST);
        happinessGain = Pet::BATH_HAPPINESS_GAIN;
    } else {
        return 400;
    }
    const int happinessTotal = happiness+happinessGain;
    levelsGained = happinessTotal/100;
    happiness = happinessTotal%100;
    Statement update(db_, "UPDATE pets SET level=level+?,happiness=?,hunger=?,energy=? WHERE user_id=?;");
    update.bind(1,levelsGained); update.bind(2,happiness); update.bind(3,hunger);
    update.bind(4,energy); update.bind(5,userId); update.done();
    if (sqlite3_changes(db_) != 1) return 404;
    transaction.commit();
    return 200;
}

int Database::equipDecoration(long long userId, int itemId) {
    const auto* item = shopItem(itemId);
    if (!item) return 404;
    if ((*item)["category"] != "decoration") return 400;
    const std::string slot = (*item)["filter"].get<std::string>();
    Transaction transaction(db_);
    {
        Statement owned(db_, "SELECT 1 FROM inventory WHERE user_id=? AND item_id=?;");
        owned.bind(1,userId); owned.bind(2,itemId);
        if (!owned.row()) return 409;
    }
    Statement equip(db_, "INSERT INTO equipped_decorations(user_id,slot,item_id) VALUES(?,?,?) "
        "ON CONFLICT(user_id,slot) DO UPDATE SET item_id=excluded.item_id;");
    equip.bind(1,userId); equip.bind(2,slot); equip.bind(3,itemId); equip.done();
    transaction.commit();
    return 200;
}

bool Database::unequipDecoration(long long userId, const std::string& slot) {
    if (slot != "headwear" && slot != "background" && slot != "floor") return false;
    Statement statement(db_, "DELETE FROM equipped_decorations WHERE user_id=? AND slot=?;");
    statement.bind(1,userId); statement.bind(2,slot); statement.done();
    return true;
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

bool Database::getStudyState(long long userId, StudyStateRow& outState) {
    Transaction transaction(db_);
    if (!ensureStudyState(db_,userId) || !loadStudyState(db_,userId,outState)) return false;
    transaction.commit();
    return true;
}

StudyOperationStatus Database::updateStudyActivity(long long userId, const std::string& action,
                                                    long long nowUnix, StudyStateRow& outState) {
    if (action != "start" && action != "heartbeat" && action != "pause")
        return StudyOperationStatus::InvalidAction;

    Transaction transaction(db_);
    if (!ensureStudyState(db_,userId) || !loadStudyState(db_,userId,outState))
        return StudyOperationStatus::UserNotFound;

    bool active = false;
    long long lastActivity = 0;
    int remainder = 0;
    {
        Statement state(db_, "SELECT is_active,IFNULL(last_activity_at,0),elapsed_remainder_seconds "
                             "FROM study_state WHERE user_id=?;");
        state.bind(1,userId);
        if (!state.row()) return StudyOperationStatus::UserNotFound;
        active = state.number(0) != 0;
        lastActivity = state.number(1);
        remainder = static_cast<int>(state.number(2));
    }

    if (active && nowUnix > lastActivity) {
        const long long elapsed = std::min<long long>(30,nowUnix-lastActivity);
        const int accumulated = remainder+static_cast<int>(elapsed);
        const int statLoss = accumulated/210;
        remainder = accumulated%210;
        if (statLoss > 0) {
            Statement decay(db_, "UPDATE pets SET hunger=MAX(0,hunger-?),energy=MAX(0,energy-?) WHERE user_id=?;");
            decay.bind(1,statLoss); decay.bind(2,statLoss); decay.bind(3,userId); decay.done();
        }
    }

    if (action == "start") active = true;
    if (action == "pause") active = false;
    if (!loadStudyState(db_,userId,outState)) return StudyOperationStatus::UserNotFound;
    if (petNeedsStudyCare(outState)) active = false;

    Statement update(db_, "UPDATE study_state SET elapsed_remainder_seconds=?,is_active=?,"
                          "last_activity_at=CASE WHEN ?=1 THEN ? ELSE NULL END WHERE user_id=?;");
    update.bind(1,remainder);
    update.bind(2,active ? 1 : 0);
    update.bind(3,active ? 1 : 0);
    update.bind(4,nowUnix);
    update.bind(5,userId);
    update.done();

    if (!loadStudyState(db_,userId,outState)) return StudyOperationStatus::UserNotFound;
    transaction.commit();
    return petNeedsStudyCare(outState) ? StudyOperationStatus::PetNeedsCare : StudyOperationStatus::Success;
}

StudyOperationStatus Database::completeStudyStage(long long userId, const std::string& level,
                                                   const std::string& tier, int correctAnswers,
                                                   StudyStateRow& outState, bool& passed, bool& review,
                                                   int& awardedPoints, int& awardedCoins) {
    static const char* levels[] = {"A1","A2","B1","B2","C1","C2"};
    static const char* tiers[] = {"easy","medium","hard"};
    int levelIndex = -1, tierIndex = -1;
    for (int i = 0; i < 6; ++i) if (level == levels[i]) levelIndex = i;
    for (int i = 0; i < 3; ++i) if (tier == tiers[i]) tierIndex = i;
    if (levelIndex < 0 || tierIndex < 0) return StudyOperationStatus::InvalidStage;
    if (correctAnswers < 0 || correctAnswers > 5) return StudyOperationStatus::InvalidAnswerCount;

    passed = studyStagePasses(correctAnswers);
    review = false;
    awardedPoints = 0;
    awardedCoins = 0;

    Transaction transaction(db_);
    if (!ensureStudyState(db_,userId) || !loadStudyState(db_,userId,outState))
        return StudyOperationStatus::UserNotFound;
    if (petNeedsStudyCare(outState)) return StudyOperationStatus::PetNeedsCare;

    const std::string stage = level+"-"+tier;
    {
        Statement existing(db_, "SELECT 1 FROM study_stage_progress WHERE user_id=? AND stage=?;");
        existing.bind(1,userId); existing.bind(2,stage);
        if (existing.row()) {
            review = true;
            transaction.commit();
            return StudyOperationStatus::Success;
        }
    }

    const int stageOrder = levelIndex*3+tierIndex;
    if (stageOrder > 0) {
        const char* previousLevel = levels[(stageOrder-1)/3];
        const char* previousTier = tiers[(stageOrder-1)%3];
        Statement previous(db_, "SELECT 1 FROM study_stage_progress WHERE user_id=? AND stage=?;");
        previous.bind(1,userId);
        previous.bind(2,std::string(previousLevel)+"-"+previousTier);
        if (!previous.row()) return StudyOperationStatus::StageLocked;
    }

    if (!passed) {
        transaction.commit();
        return StudyOperationStatus::Success;
    }

    awardedPoints = correctAnswers*studyPointsPerCorrectAnswer(tier)+studyCompletionBonus(tier);
    awardedCoins = studyCompletionCoins(tier);
    Statement record(db_, "INSERT INTO study_stage_progress(user_id,stage,stage_order,points_awarded,coins_awarded) "
                          "VALUES(?,?,?,?,?);");
    record.bind(1,userId); record.bind(2,stage); record.bind(3,stageOrder);
    record.bind(4,awardedPoints); record.bind(5,awardedCoins); record.done();

    int happinessRemainder = 0;
    {
        Statement current(db_, "SELECT happiness_remainder_points FROM study_state WHERE user_id=?;");
        current.bind(1,userId);
        if (!current.row()) return StudyOperationStatus::UserNotFound;
        happinessRemainder = static_cast<int>(current.number(0));
    }
    const int happinessTotal = happinessRemainder+awardedPoints;
    const int happinessGain = happinessTotal/50;
    Statement studyUpdate(db_, "UPDATE study_state SET total_points=total_points+?,"
                               "happiness_remainder_points=? WHERE user_id=?;");
    studyUpdate.bind(1,awardedPoints); studyUpdate.bind(2,happinessTotal%50); studyUpdate.bind(3,userId);
    studyUpdate.done();

    Statement petUpdate(db_, "UPDATE pets SET level=level+((happiness+?)/100),"
                             "happiness=((happiness+?)%100) WHERE user_id=?;");
    petUpdate.bind(1,happinessGain); petUpdate.bind(2,happinessGain); petUpdate.bind(3,userId); petUpdate.done();
    Statement coinUpdate(db_, "UPDATE users SET coins=coins+? WHERE id=?;");
    coinUpdate.bind(1,awardedCoins); coinUpdate.bind(2,userId); coinUpdate.done();

    if (!loadStudyState(db_,userId,outState)) return StudyOperationStatus::UserNotFound;
    transaction.commit();
    return StudyOperationStatus::Success;
}

bool Database::getPracticeState(long long userId, PracticeStateRow& outState) {
    Transaction transaction(db_);
    if (!ensurePracticeState(db_,userId) || !loadPracticeState(db_,userId,outState)) return false;
    transaction.commit();
    return true;
}

PracticeOperationStatus Database::updatePracticeActivity(long long userId, const std::string& action,
                                                          long long nowUnix, PracticeStateRow& outState) {
    if (action != "start" && action != "heartbeat" && action != "pause")
        return PracticeOperationStatus::InvalidAction;

    Transaction transaction(db_);
    if (!ensurePracticeState(db_,userId) || !loadPracticeState(db_,userId,outState))
        return PracticeOperationStatus::UserNotFound;

    bool active = false;
    long long lastActivity = 0;
    int remainder = 0;
    {
        Statement state(db_, "SELECT is_active,IFNULL(last_activity_at,0),elapsed_remainder_seconds "
                             "FROM practice_state WHERE user_id=?;");
        state.bind(1,userId);
        if (!state.row()) return PracticeOperationStatus::UserNotFound;
        active = state.number(0) != 0;
        lastActivity = state.number(1);
        remainder = static_cast<int>(state.number(2));
    }

    if (active && nowUnix > lastActivity) {
        const long long elapsed = std::min<long long>(30,nowUnix-lastActivity);
        const int accumulated = remainder+static_cast<int>(elapsed);
        const int statLoss = accumulated/210;
        remainder = accumulated%210;
        if (statLoss > 0) {
            Statement decay(db_, "UPDATE pets SET hunger=MAX(0,hunger-?),energy=MAX(0,energy-?) WHERE user_id=?;");
            decay.bind(1,statLoss); decay.bind(2,statLoss); decay.bind(3,userId); decay.done();
        }
    }

    if (action == "start") active = true;
    if (action == "pause") active = false;
    Statement update(db_, "UPDATE practice_state SET elapsed_remainder_seconds=?,is_active=?,"
                          "last_activity_at=CASE WHEN ?=1 THEN ? ELSE NULL END WHERE user_id=?;");
    update.bind(1,remainder);
    update.bind(2,active ? 1 : 0);
    update.bind(3,active ? 1 : 0);
    update.bind(4,nowUnix);
    update.bind(5,userId);
    update.done();

    if (!loadPracticeState(db_,userId,outState)) return PracticeOperationStatus::UserNotFound;
    transaction.commit();
    return PracticeOperationStatus::Success;
}

bool Database::startPracticeGame(long long userId, const std::string& gameId) {
    Transaction transaction(db_);
    if (!ensurePracticeState(db_,userId)) return false;
    Statement update(db_, "UPDATE practice_state SET active_game_id=? WHERE user_id=?;");
    update.bind(1,gameId); update.bind(2,userId); update.done();
    transaction.commit();
    return true;
}

bool Database::abandonPracticeGame(long long userId, const std::string& gameId) {
    Transaction transaction(db_);
    if (!ensurePracticeState(db_,userId)) return false;
    Statement update(db_, "UPDATE practice_state SET active_game_id='' "
                          "WHERE user_id=? AND active_game_id=?;");
    update.bind(1,userId); update.bind(2,gameId); update.done();
    transaction.commit();
    return true;
}

PracticeOperationStatus Database::finishPracticeGame(long long userId, const std::string& gameId,
                                                      int eaten, PracticeStateRow& outState,
                                                      int& awardedPoints, int& awardedCoins,
                                                      bool& duplicate) {
    awardedPoints = 0;
    awardedCoins = 0;
    duplicate = false;
    Transaction transaction(db_);
    if (!ensurePracticeState(db_,userId) || !loadPracticeState(db_,userId,outState))
        return PracticeOperationStatus::UserNotFound;

    {
        Statement previous(db_, "SELECT points_awarded,coins_awarded FROM practice_game_rewards "
                                "WHERE user_id=? AND game_id=?;");
        previous.bind(1,userId); previous.bind(2,gameId);
        if (previous.row()) {
            awardedPoints = static_cast<int>(previous.number(0));
            awardedCoins = static_cast<int>(previous.number(1));
            duplicate = true;
            if (!loadPracticeState(db_,userId,outState)) return PracticeOperationStatus::UserNotFound;
            transaction.commit();
            return PracticeOperationStatus::Success;
        }
    }

    std::string activeGameId;
    int happinessRemainder = 0;
    int previousBest = 0;
    {
        Statement state(db_, "SELECT active_game_id,happiness_remainder_points,best_score "
                             "FROM practice_state WHERE user_id=?;");
        state.bind(1,userId);
        if (!state.row()) return PracticeOperationStatus::UserNotFound;
        activeGameId = state.text(0);
        happinessRemainder = static_cast<int>(state.number(1));
        previousBest = static_cast<int>(state.number(2));
    }
    if (activeGameId != gameId) return PracticeOperationStatus::GameNotActive;

    awardedPoints = (eaten/5)*15;
    awardedCoins = previousBest > 0 && eaten > previousBest ? 15 : 0;
    const int happinessTotal = happinessRemainder+awardedPoints;
    const int happinessGain = happinessTotal/20;

    Statement record(db_, "INSERT INTO practice_game_rewards(user_id,game_id,eaten,points_awarded,coins_awarded) "
                          "VALUES(?,?,?,?,?);");
    record.bind(1,userId); record.bind(2,gameId); record.bind(3,eaten);
    record.bind(4,awardedPoints); record.bind(5,awardedCoins); record.done();

    Statement stateUpdate(db_, "UPDATE practice_state SET total_points=total_points+?,"
                               "happiness_remainder_points=?,best_score=MAX(best_score,?),"
                               "active_game_id='' "
                               "WHERE user_id=?;");
    stateUpdate.bind(1,awardedPoints); stateUpdate.bind(2,happinessTotal%20);
    stateUpdate.bind(3,eaten); stateUpdate.bind(4,userId);
    stateUpdate.done();

    Statement petUpdate(db_, "UPDATE pets SET level=level+((happiness+?)/100),"
                             "happiness=((happiness+?)%100) WHERE user_id=?;");
    petUpdate.bind(1,happinessGain); petUpdate.bind(2,happinessGain); petUpdate.bind(3,userId); petUpdate.done();
    Statement coinUpdate(db_, "UPDATE users SET coins=coins+? WHERE id=?;");
    coinUpdate.bind(1,awardedCoins); coinUpdate.bind(2,userId); coinUpdate.done();

    if (!loadPracticeState(db_,userId,outState)) return PracticeOperationStatus::UserNotFound;
    transaction.commit();
    return PracticeOperationStatus::Success;
}

bool Database::createCodeSubmission(Submission& submission) {
    try {
        Statement statement(db_, "INSERT INTO code_submissions(user_id,exercise_id,source_code,language,status,score,message,submitted_at,finished_at) VALUES(?,?,?,?,?,?,?,?,?);");
        statement.bind(1, submission.userId);
        statement.bind(2, submission.exerciseId);
        statement.bind(3, submission.sourceCode);
        statement.bind(4, submission.language);
        statement.bind(5, std::string(toString(submission.status)));
        statement.bind(6, static_cast<long long>(submission.score));
        statement.bind(7, submission.message);
        statement.bind(8, submission.submittedAt);
        statement.bind(9, submission.finishedAt);
        statement.done();
        submission.id = sqlite3_last_insert_rowid(db_);
        return true;
    } catch (const std::exception& error) { lastError_ = error.what(); return false; }
}

bool Database::getCodeSubmission(long long submissionId, Submission& outSubmission) {
    try {
        Statement statement(db_, "SELECT id,user_id,exercise_id,source_code,language,status,score,message,submitted_at,finished_at FROM code_submissions WHERE id=?;");
        statement.bind(1, submissionId);
        if (!statement.row()) return false;
        outSubmission.id = statement.number(0);
        outSubmission.userId = statement.number(1);
        outSubmission.exerciseId = statement.number(2);
        outSubmission.sourceCode = statement.text(3);
        outSubmission.language = statement.text(4);
        if (!submissionStatusFromString(statement.text(5), outSubmission.status)) outSubmission.status = SubmissionStatus::InternalError;
        outSubmission.score = static_cast<int>(statement.number(6));
        outSubmission.message = statement.text(7);
        outSubmission.submittedAt = statement.text(8);
        outSubmission.finishedAt = statement.text(9);
        return true;
    } catch (const std::exception& error) { lastError_ = error.what(); return false; }
}
