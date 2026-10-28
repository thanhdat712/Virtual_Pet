#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#include <string>
#include <vector>
#include "../Exercise/Submission.h"

struct sqlite3;

// Dữ liệu người dùng (không bao gồm hash mật khẩu khi trả ra ngoài).
struct UserRow {
    long long id = 0;
        std::string username;
    std::string email;
    int coins = 0;
        bool soundEnabled = true;
    bool emailVerified = false;
};

struct PetRow {
    long long userId = 0;
    std::string name = "UIA";
        std::string species = "cat";
    bool speciesChosen = false;
    int level = 1;
    int experience = 0;
    int happiness = 85;
    int hunger = 30;
    int energy = 70;
    int sleepRemainingSeconds = 0;
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

struct StudyStateRow {
    long long points = 0;
    int coins = 0;
    int level = 1;
    int happiness = 0;
    int hunger = 50;
    int energy = 50;
    std::vector<std::string> completedStages;
};

struct PracticeStateRow {
    long long points = 0;
    int bestScore = 0;
    int coins = 0;
    int level = 1;
    int happiness = 0;
    int hunger = 50;
    int energy = 50;
};

enum class StudyOperationStatus {
    Success,
    PetNeedsCare,
    StageLocked,
    InvalidStage,
    InvalidAnswerCount,
    InvalidAction,
    UserNotFound
};

enum class PracticeOperationStatus {
    Success,
    InvalidAction,
    GameNotActive,
    UserNotFound
};

struct InventoryRow {
    int id = 0;
    int quantity = 0;
};

    struct EquipmentRow {
        std::string slot;
        int itemId = 0;
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
    bool issueEmailVerification(const std::string& email, const std::string& tokenHash, long long expiresAt);
    bool verifyEmail(const std::string& email, const std::string& tokenHash);

    bool createPendingEmailChange(long long userId, const std::string& email,
                                  const std::string& tokenHash, long long expiresAt);
    bool confirmEmailChange(long long userId, const std::string& email,
                            const std::string& tokenHash);

    bool createPasswordReset(const std::string& email, const std::string& tokenHash, long long expiresAt);
    bool resetPassword(const std::string& tokenHash, const std::string& salt,
                       const std::string& passwordHash);
    bool changePassword(long long userId, const std::string& currentPassword,
                        const std::string& newPassword, const std::string& currentSessionHash);
        bool usernameExists(const std::string& username);
    // Tạo user + pet mặc định. Trả về true và điền outUser nếu thành công.
    bool createUser(const std::string& email, const std::string& password, UserRow& outUser);
        bool createUser(const std::string& username, const std::string& email,
                        const std::string& password, UserRow& outUser);
    // Kiểm tra đăng nhập; true nếu đúng mật khẩu, điền outUser.
    bool verifyLogin(const std::string& email, const std::string& password, UserRow& outUser);
    bool createPasswordReset(const std::string& email, const std::string& tokenHash, long long expiresAt);
    bool resetPassword(const std::string& tokenHash, const std::string& salt, const std::string& passwordHash);
    bool changePassword(long long userId, const std::string& currentPassword,
                        const std::string& newPassword, const std::string& currentSessionHash);
    bool touchLastLogin(long long userId);

    std::string createSession(long long userId);
    bool userIdForToken(const std::string& token, long long& outUserId);
    bool revokeSession(const std::string& token);

    bool getUserById(long long userId, UserRow& outUser);
        bool setSoundEnabled(long long userId, bool enabled);
    std::vector<InventoryRow> listInventory(long long userId);
        std::vector<EquipmentRow> listEquipment(long long userId);
    int purchase(long long userId, int itemId, int quantity);

    // --- Pet ---
    bool getPet(long long userId, PetRow& outPet);
    bool renamePet(long long userId, const std::string& name);
        bool savePetProfile(long long userId, const std::string& name, const std::string& species);
    int feed(long long userId, int itemId);
        int performPetAction(long long userId, const std::string& action, int& levelsGained);
        int equipDecoration(long long userId, int itemId);
        bool unequipDecoration(long long userId, const std::string& slot);

    // --- Bài tập & tiến độ ---
    std::vector<ExerciseRow> listExercises();
    bool exerciseExists(long long exerciseId);
    std::vector<ProgressRow> listProgress(long long userId);
    // Ghi một lần nộp bài và cập nhật tiến độ (best_score, attempts, status).
    bool recordSubmission(long long userId, long long exerciseId, const std::string& answer,
                          int& score, ProgressRow& outProgress);

    // --- Study quiz progression and visible-time stat decay ---
    bool getStudyState(long long userId, StudyStateRow& outState);
    StudyOperationStatus updateStudyActivity(long long userId, const std::string& action,
                                              long long nowUnix, StudyStateRow& outState);
    StudyOperationStatus completeStudyStage(long long userId, const std::string& level,
                                             const std::string& tier, int correctAnswers,
                                             StudyStateRow& outState, bool& passed, bool& review,
                                             int& awardedPoints, int& awardedCoins);

    // --- Practice mini-game rewards, account record, and page-time stat decay ---
    bool getPracticeState(long long userId, PracticeStateRow& outState);
    PracticeOperationStatus updatePracticeActivity(long long userId, const std::string& action,
                                                    long long nowUnix, PracticeStateRow& outState);
    bool startPracticeGame(long long userId, const std::string& gameId);
    bool abandonPracticeGame(long long userId, const std::string& gameId);
    PracticeOperationStatus finishPracticeGame(long long userId, const std::string& gameId,
                                                int eaten, PracticeStateRow& outState,
                                                int& awardedPoints, int& awardedCoins,
                                                bool& duplicate);

    // --- Bài nộp mã nguồn (SUB) ---
    bool createCodeSubmission(Submission& submission);
    bool getCodeSubmission(long long submissionId, Submission& outSubmission);

private:
    bool exec(const std::string& sql);
    sqlite3* db_ = nullptr;
    std::string lastError_;
};

#endif
