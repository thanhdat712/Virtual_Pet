#ifndef PET_REPOSITORY_H
#define PET_REPOSITORY_H

#include "IPetRepository.h"
#include "DatabaseConnection.h"

// Triển khai IPetRepository sử dụng SQLite và Prepared Statements
class PetRepository : public IPetRepository {
private:
    DatabaseConnection& db;

public:
    explicit PetRepository(DatabaseConnection& dbConnection);

    std::optional<PetSnapshot> getPetByUserId(int userId) override;
    bool savePet(int userId, const PetSnapshot& pet) override;
    bool renamePet(int userId, const std::string& name) override;
    bool updateStats(int userId, int health, int happiness, int hunger, int energy, int level, int experience = 0) override;
};

#endif
