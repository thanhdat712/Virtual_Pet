#ifndef I_PET_REPOSITORY_H
#define I_PET_REPOSITORY_H

#include <string>
#include <optional>
#include "../Core/GameSnapshot.h"

// Interface định nghĩa các thao tác lưu trữ thú cưng theo Mẫu Repository
class IPetRepository {
public:
    virtual ~IPetRepository() = default;

    virtual std::optional<PetSnapshot> getPetByUserId(int userId) = 0;
    virtual bool savePet(int userId, const PetSnapshot& pet) = 0;
    virtual bool renamePet(int userId, const std::string& name) = 0;
    virtual bool updateStats(int userId, int health, int happiness, int hunger, int energy, int level, int experience = 0) = 0;
};

#endif
