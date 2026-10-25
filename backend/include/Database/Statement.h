#ifndef DATABASE_STATEMENT_H
#define DATABASE_STATEMENT_H

#include <string>
#include <stdexcept>
#include <cstdint>

struct sqlite3;
struct sqlite3_stmt;

// Lớp bao bọc Prepared Statement SQLite theo mô hình RAII.
// Ngăn chặn SQL Injection tuyệt đối bằng cách buộc tham số qua sqlite3_bind_*.
class Statement {
private:
    sqlite3_stmt* stmt = nullptr;
    sqlite3* dbHandle = nullptr;

public:
    Statement(sqlite3* db, const std::string& sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    // Ràng buộc tham số (1-based index)
    void bindInt(int index, int value);
    void bindInt64(int index, std::int64_t value);
    void bindDouble(int index, double value);
    void bindString(int index, const std::string& value);
    void bindNull(int index);

    // Thực thi
    bool step(); // Trả về true nếu có dòng kết quả (SQLITE_ROW), false khi kết thúc (SQLITE_DONE)
    void execute(); // Thực thi câu lệnh không cần đọc kết quả (INSERT/UPDATE/DELETE)

    // Đọc cột (0-based index)
    int getInt(int column) const;
    std::int64_t getInt64(int column) const;
    double getDouble(int column) const;
    std::string getString(int column) const;
    bool isNull(int column) const;

    void reset();
    void clearBindings();

    sqlite3_stmt* getRawStatement() const { return stmt; }
};

#endif
