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
#include "Pet.h"
#include <algorithm>

int Pet::clampStat(int value) {
    return std::max(MIN_STAT, std::min(MAX_STAT, value));
}

Pet::Pet(int id, const std::string& name, int level, int health, int happiness, int hunger, int energy)
    : id(id), name(name), level(std::max(1, level)), health(clampStat(health)),
      happiness(clampStat(happiness)), hunger(clampStat(hunger)), energy(clampStat(energy)) {}

int Pet::getId() const { return id; }
const std::string& Pet::getName() const { return name; }
int Pet::getLevel() const { return level; }
int Pet::getHealth() const { return health; }
int Pet::getHappiness() const { return happiness; }
int Pet::getHunger() const { return hunger; }
int Pet::getEnergy() const { return energy; }

void Pet::setName(const std::string& newName) { if (!newName.empty()) name = newName; }
void Pet::setHealth(int value) { health = clampStat(value); }
void Pet::setHappiness(int value) { happiness = clampStat(value); }
void Pet::setHunger(int value) { hunger = clampStat(value); }
void Pet::setEnergy(int value) { energy = clampStat(value); }
void Pet::changeHealth(int amount) { setHealth(health + amount); }
void Pet::changeHappiness(int amount) { setHappiness(happiness + amount); }
void Pet::changeHunger(int amount) { setHunger(hunger + amount); }
void Pet::changeEnergy(int amount) { setEnergy(energy + amount); }

bool Pet::eat(const Food& food) {
    if (health == 0) return false;
    changeHunger(-food.getHungerRestore());
    changeHappiness(food.getHappinessRestore());
    return true;
}

void Pet::play() {
    if (energy < 10 || health == 0) return;
    changeHappiness(15);
    changeEnergy(-10);
    changeHunger(10);
}

void Pet::sleep() {
    if (health == 0) return;
    changeEnergy(30);
    changeHealth(5);
    changeHunger(5);
}

void Pet::bath() {
    if (health == 0) return;
    changeHealth(5);
    changeHappiness(5);
    changeEnergy(-5);
}

void Pet::updateOverTime(int minutes) {
    if (minutes <= 0) return;
    // Moi 10 phut: doi tang 1, nang luong giam 1, vui giam 1.
    const int ticks = minutes / 10;
    changeHunger(ticks);
    changeEnergy(-ticks);
    changeHappiness(-ticks);
    if (hunger >= 100) changeHealth(-(ticks / 2));
}
