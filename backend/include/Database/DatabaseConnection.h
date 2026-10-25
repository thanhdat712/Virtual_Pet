#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <string>
#include <memory>

struct sqlite3;

// Quản lý kết nối SQLite RAII. Hỗ trợ cả file trên đĩa và database trong bộ nhớ (":memory:").
class DatabaseConnection {
private:
    sqlite3* db = nullptr;
    std::string dbPath;
    std::string lastErrorMessage;

public:
    DatabaseConnection();
    ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    DatabaseConnection(DatabaseConnection&& other) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&& other) noexcept;

    // Mở kết nối SQLite (truyền ":memory:" để test trong bộ nhớ)
    bool open(const std::string& path = ":memory:");
    void close();

    bool isOpen() const;
    sqlite3* getRawHandle() const;
    const std::string& getPath() const;
    const std::string& getLastError() const;

    // Thực thi câu lệnh SQL trực tiếp (thường dùng cho DDL, PRAGMA)
    bool execute(const std::string& sql);

    // Khởi tạo schema mặc định cho game (users, player_profiles, pets, inventory, sessions)
    bool initializeSchema();
};

#endif
