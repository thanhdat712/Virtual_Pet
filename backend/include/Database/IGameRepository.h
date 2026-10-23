#ifndef I_GAME_REPOSITORY_H
#define I_GAME_REPOSITORY_H

#include <optional>
#include <string>
#include "../Core/GameSnapshot.h"

// Interface Repository cho toàn bộ trạng thái GameSnapshot (KAN-72 / INT-02)
class IGameRepository {
public:
    virtual ~IGameRepository() = default;

    // Nạp toàn bộ trạng thái game (người chơi, pet, túi đồ, trang bị)
    virtual std::optional<GameSnapshot> loadGame(int userId) = 0;

    // Lưu toàn bộ trạng thái game vào SQLite trong MỘT GIAO DỊCH (Transaction).
    // Nếu có bất kỳ thao tác nào thất bại, tự động ROLLBACK hoàn toàn.
    virtual bool saveGame(const GameSnapshot& snapshot) = 0;

    // Khởi tạo dữ liệu game ban đầu cho một user mới (100 coins, pet mặc định)
    virtual bool createInitialGame(int userId, const std::string& playerName = "Player") = 0;
};

#endif
