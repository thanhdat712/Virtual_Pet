#include "Pet.h"
#include "Food.h"
#include <algorithm>

namespace {
int clampStatValue(long long value) {
    if (value < Pet::MIN_STAT) return Pet::MIN_STAT;
    if (value > Pet::MAX_STAT) return Pet::MAX_STAT;
    return static_cast<int>(value);
    }
}

Pet::Pet(int id, const std::string& name,
         int level, int health, int happiness,
         int hunger, int energy, int experience)
    : id(id),
      name(name),
      level(std::max(1, level)),
      health(clampStatValue(health)),
      happiness(clampStatValue(happiness)),
      hunger(clampStatValue(hunger)),
      energy(clampStatValue(energy)),
      experience(std::max(0, experience)) {}

int Pet::clampStat(int value) {
    return std::max(MIN_STAT, std::min(MAX_STAT, value));
}

int Pet::getId() const { return id; }
const std::string& Pet::getName() const { return name; }
int Pet::getLevel() const { return level; }
int Pet::getHealth() const { return health; }
int Pet::getHappiness() const { return happiness; }
int Pet::getHunger() const { return hunger; }
int Pet::getEnergy() const { return energy; }
int Pet::getExperience() const { return experience; }

void Pet::setName(const std::string& newName) {
    if (!newName.empty()) name = newName;
}

void Pet::setHealth(int value) {
    health = clampStat(value);
}

void Pet::setHappiness(int value) {
    happiness = clampStat(value);
}

void Pet::setHunger(int value) {
    hunger = clampStat(value);
}

void Pet::setEnergy(int value) {
    energy = clampStat(value);
}

void Pet::setExperience(int value) {
    experience = std::max(0, value);
}

void Pet::changeHealth(int amount) {
    setHealth(health + amount);
}

void Pet::changeHappiness(int amount) {
    setHappiness(happiness + amount);
}

void Pet::changeHunger(int amount) {
    setHunger(hunger + amount);
}

void Pet::changeEnergy(int amount) {
    setEnergy(energy + amount);
}

int Pet::addHappiness(int amount) {
    changeHappiness(amount);
    return 0;
}

ActionResult Pet::feed(const Food& food) {
    if (health <= 0) {
        return ActionResult::failure(
            ActionCode::PET_TOO_TIRED, "Pet cannot eat."
        );
    }

    changeHunger(food.getHungerRestore());
    int gained = addHappiness(food.getHappinessRestore());

    return ActionResult::success(food.getName() + " eaten!", gained);
}

ActionResult Pet::play() {
    if (health <= 0 || energy < PLAY_ENERGY_COST) {
        return ActionResult::failure(
            ActionCode::PET_TOO_TIRED,
            "Your pet is too tired to play."
        );
    }

    changeEnergy(-PLAY_ENERGY_COST);
    changeHunger(-PLAY_HUNGER_COST);
    int gained = addHappiness(PLAY_HAPPINESS_GAIN);

    return ActionResult::success("Your pet had fun!", gained);
}

ActionResult Pet::sleep() {
    if (health <= 0 || energy >= MAX_STAT) {
        return ActionResult::failure(
            ActionCode::ENERGY_FULL,
            "Your pet cannot sleep now."
        );
    }

    changeEnergy(SLEEP_ENERGY_GAIN);
    changeHunger(-SLEEP_HUNGER_COST);

    return ActionResult::success("Your pet slept well.");
}

ActionResult Pet::bath() {
    if (health <= 0) {
        return ActionResult::failure(
            ActionCode::PET_TOO_TIRED,
            "Your pet cannot take a bath."
        );
    }

    changeEnergy(-BATH_ENERGY_COST);
    addHappiness(BATH_HAPPINESS_GAIN);

    return ActionResult::success("Squeaky clean!");
}

bool Pet::eat(const Food& food) {
    return feed(food).ok();
}

void Pet::updateOverTime(int minutes) {
    if (minutes <= 0) return;

    int ticks = minutes / 10;

    changeHunger(-ticks);
    changeEnergy(-ticks);
    changeHappiness(-ticks);

    if (hunger >= MAX_STAT) {
        changeHealth(-(ticks / 2));
    }
}

void Pet::applyTimePassed(int ticks) {
    if (ticks <= 0) return;

    for (int i = 0; i < ticks; ++i) {
        changeHunger(-HUNGER_DECAY_PER_TICK);
        changeEnergy(-ENERGY_DECAY_PER_TICK);
    }
}

void Pet::equip(DecorationSlot slot, int itemId) {
    if (itemId > 0) {
        equipment[slot] = itemId;
    }
}

void Pet::unequip(DecorationSlot slot) {
    equipment.erase(slot);
}

int Pet::getEquipped(DecorationSlot slot) const {
    auto it = equipment.find(slot);
    return it == equipment.end() ? 0 : it->second;
}

const std::map<DecorationSlot, int>& Pet::getEquipment() const {
    return equipment;
}