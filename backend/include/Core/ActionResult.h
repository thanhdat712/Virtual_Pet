#ifndef ACTION_RESULT_H
#define ACTION_RESULT_H

#include <string>

// Mã kết quả của mọi hành động người chơi (feed/play/sleep/bath/buy/decorate...)
// và các thao tác đổi trạng thái game. Tầng API dùng toString() để trả mã lỗi cho frontend.
enum class ActionCode {
    OK,
    INVALID_STATE,      // game không ở trạng thái cho phép làm việc này (chưa start, đang pause...)
    INVALID_ARGUMENT,   // tham số sai (số lượng < 1, thời gian âm...)
    ITEM_NOT_FOUND,     // không có item với id này
    NOT_FOOD,           // item không phải đồ ăn
    NOT_DECORATION,     // item không phải đồ trang trí
    NOT_OWNED,          // người chơi không có item này trong túi
    ALREADY_OWNED,      // đồ trang trí chỉ được sở hữu 1 cái
    NOT_ENOUGH_COINS,   // không đủ xu
    PET_TOO_TIRED,      // pet quá mệt để chơi
    ENERGY_FULL,        // năng lượng đã đầy, không cần ngủ
    INVALID_SNAPSHOT    // dữ liệu nạp vào không hợp lệ
};

inline const char* toString(ActionCode code) {
    switch (code) {
        case ActionCode::OK:               return "OK";
        case ActionCode::INVALID_STATE:    return "INVALID_STATE";
        case ActionCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ActionCode::ITEM_NOT_FOUND:   return "ITEM_NOT_FOUND";
        case ActionCode::NOT_FOOD:         return "NOT_FOOD";
        case ActionCode::NOT_DECORATION:   return "NOT_DECORATION";
        case ActionCode::NOT_OWNED:        return "NOT_OWNED";
        case ActionCode::ALREADY_OWNED:    return "ALREADY_OWNED";
        case ActionCode::NOT_ENOUGH_COINS: return "NOT_ENOUGH_COINS";
        case ActionCode::PET_TOO_TIRED:    return "PET_TOO_TIRED";
        case ActionCode::ENERGY_FULL:      return "ENERGY_FULL";
        case ActionCode::INVALID_SNAPSHOT: return "INVALID_SNAPSHOT";
    }
    return "UNKNOWN";
}

// Kết quả trả về của một hành động: thành công hay lỗi gì, kèm thông báo cho người chơi.
struct ActionResult {
    ActionCode code = ActionCode::OK;
    std::string message;
    int levelsGained = 0; // số cấp pet vừa lên do hành động này (feed/play/bath)

    bool ok() const { return code == ActionCode::OK; }

    static ActionResult success(const std::string& message = "", int levelsGained = 0) {
        return ActionResult{ActionCode::OK, message, levelsGained};
    }
    static ActionResult failure(ActionCode code, const std::string& message) {
        return ActionResult{code, message, 0};
    }
};

#endif
