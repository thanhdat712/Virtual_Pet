#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#include <string>
#include <vector>

struct sqlite3;

// Dữ liệu người dùng (không bao gồm hash mật khẩu khi trả ra ngoài).
struct UserRow {
    long long id = 0;
    std::string email;
    int coins = 0;
};

struct PetRow {
    long long userId = 0;
    std::string name = "UIA";
    std::string species = "cat";
    int level = 1;
    int experience = 0;
    int happiness = 85;
    int hunger = 30;
    int energy = 70;
};

struct ExerciseRow {
    long long id = 0;
    std::string title;
    std::string description;
    std::string difficulty;
    int maxScore = 100;
};

struct ProgressRow {
    long long exerciseId = 0;
    std::string status;
    int bestScore = 0;
    int attempts = 0;
    std::string completedAt;
};

struct InventoryRow {
    int id = 0;
    int quantity = 0;
};

// Bao bọc kết nối SQLite + các thao tác CRUD cho API.
// Mọi câu lệnh đều dùng prepared statement với tham số buộc (chống SQL injection).
class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& path);
    void close();
    bool runSchema();
    bool seedExercisesIfEmpty();

    // Trả về chuỗi lỗi gần nhất (để log).
    std::string lastError() const { return lastError_; }

    // --- Người dùng & phiên đăng nhập ---
    bool emailExists(const std::string& email);
    // Tạo user + pet mặc định. Trả về true và điền outUser nếu thành công.
    bool createUser(const std::string& email, const std::string& password, UserRow& outUser);
    // Kiểm tra đăng nhập; true nếu đúng mật khẩu, điền outUser.
    bool verifyLogin(const std::string& email, const std::string& password, UserRow& outUser);
    bool touchLastLogin(long long userId);

    std::string createSession(long long userId);
    bool userIdForToken(const std::string& token, long long& outUserId);
    bool revokeSession(const std::string& token);

    bool getUserById(long long userId, UserRow& outUser);
    std::vector<InventoryRow> listInventory(long long userId);
    int purchase(long long userId, int itemId, int quantity);

    // --- Pet ---
    bool getPet(long long userId, PetRow& outPet);
    bool renamePet(long long userId, const std::string& name);
    bool setPetSpecies(long long userId, const std::string& species);
    int feed(long long userId, int itemId);

    // --- Bài tập & tiến độ ---
    std::vector<ExerciseRow> listExercises();
    bool exerciseExists(long long exerciseId);
    std::vector<ProgressRow> listProgress(long long userId);
    // Ghi một lần nộp bài và cập nhật tiến độ (best_score, attempts, status).
    bool recordSubmission(long long userId, long long exerciseId, const std::string& answer,
                          int& score, ProgressRow& outProgress);

private:
    bool exec(const std::string& sql);
    sqlite3* db_ = nullptr;
    std::string lastError_;
};

#endif
