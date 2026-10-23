#ifndef GAME_REPOSITORY_H
#define GAME_REPOSITORY_H

#include "IGameRepository.h"
#include "DatabaseConnection.h"
#include "UserRepository.h"
#include "PetRepository.h"
#include "InventoryRepository.h"

// Triển khai IGameRepository kết hợp các repository con trong một Transaction trọn vẹn
class GameRepository : public IGameRepository {
private:
    DatabaseConnection& db;
    UserRepository userRepo;
    PetRepository petRepo;
    InventoryRepository inventoryRepo;

public:
    explicit GameRepository(DatabaseConnection& dbConnection);

    std::optional<GameSnapshot> loadGame(int userId) override;
    bool saveGame(const GameSnapshot& snapshot) override;
    bool createInitialGame(int userId, const std::string& playerName = "Player") override;

    UserRepository& getUserRepository() { return userRepo; }
    PetRepository& getPetRepository() { return petRepo; }
    InventoryRepository& getInventoryRepository() { return inventoryRepo; }
};

#endif
