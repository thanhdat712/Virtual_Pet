#include "Pet.h"

namespace {
int clampStat(int value) {
    if (value < Pet::MIN_STAT) return Pet::MIN_STAT;
    if (value > Pet::MAX_STAT) return Pet::MAX_STAT;
    return value;
}
}

Pet::Pet(int id, const std::string& name, int health, int happiness, int hunger, int energy)
    : id(id),
      name(name),
      health(clampStat(health)),
      happiness(clampStat(happiness)),
      hunger(clampStat(hunger)),
      energy(clampStat(energy)) {}

int Pet::getId() const { return id; }
const std::string& Pet::getName() const { return name; }
int Pet::getHealth() const { return health; }
int Pet::getHappiness() const { return happiness; }
int Pet::getHunger() const { return hunger; }
int Pet::getEnergy() const { return energy; }
