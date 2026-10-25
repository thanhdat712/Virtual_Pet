#ifndef DATABASE_TRANSACTION_H
#define DATABASE_TRANSACTION_H

#include <string>

struct sqlite3;

// Quản lý giao dịch (Transaction) theo mô hình RAII.
// Đảm bảo tính toàn vẹn (ACID): nếu có lỗi hoặc ngoại lệ xảy ra trước khi commit(),
// destructor sẽ tự động ROLLBACK để đưa database về trạng thái an toàn nguyên vẹn.
class Transaction {
private:
    sqlite3* db = nullptr;
    bool committed = false;
    bool rolledBack = false;

public:
    explicit Transaction(sqlite3* db, bool immediate = true);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;

    // Xác nhận và lưu các thay đổi vĩnh viễn
    void commit();

    // Hủy bỏ toàn bộ các thay đổi trong giao dịch
    void rollback();

    bool isCompleted() const { return committed || rolledBack; }
};

#endif
