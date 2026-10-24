#include "Pet.h"
#include "Food.h"

namespace {
int clampStat(long long value) {
    if (value < Pet::MIN_STAT) return Pet::MIN_STAT;
    if (value > Pet::MAX_STAT) return Pet::MAX_STAT;
    return static_cast<int>(value);
}
}

Pet::Pet(int id, const std::string& name, int health, int happiness, int hunger, int energy, int level)
    : id(id),
      name(name),
      level(level < 1 ? 1 : level),
      health(clampStat(health)),
      happiness(clampStat(happiness)),
      hunger(clampStat(hunger)),
      energy(clampStat(energy)) {}

int Pet::getId() const { return id; }
const std::string& Pet::getName() const { return name; }
int Pet::getLevel() const { return level; }
int Pet::getHealth() const { return health; }
int Pet::getHappiness() const { return happiness; }
int Pet::getHunger() const { return hunger; }
int Pet::getEnergy() const { return energy; }

int Pet::addHappiness(int amount) {
    int levelsGained = 0;
    long long value = static_cast<long long>(happiness) + amount;
    if (value < MIN_STAT) value = MIN_STAT;
    while (value >= MAX_STAT) { // thanh vui đầy -> lên cấp, giữ phần dư
        value -= MAX_STAT;
        level++;
        levelsGained++;
    }
    happiness = static_cast<int>(value);
    return levelsGained;
}

ActionResult Pet::feed(const Food& food) {
    hunger = clampStat(static_cast<long long>(hunger) + food.getHungerRestore());
    int gained = addHappiness(food.getHappinessRestore());
    return ActionResult::success(food.getName() + " eaten!", gained);
}

ActionResult Pet::play() {
    if (energy < PLAY_ENERGY_COST) {
        return ActionResult::failure(ActionCode::PET_TOO_TIRED, "Your pet is too tired to play.");
    }
    energy = clampStat(static_cast<long long>(energy) - PLAY_ENERGY_COST);
    hunger = clampStat(static_cast<long long>(hunger) - PLAY_HUNGER_COST);
    int gained = addHappiness(PLAY_HAPPINESS_GAIN);
    return ActionResult::success("Your pet had fun!", gained);
}

ActionResult Pet::sleep() {
    if (energy >= MAX_STAT) {
        return ActionResult::failure(ActionCode::ENERGY_FULL, "Your pet is not sleepy.");
    }
    energy = clampStat(static_cast<long long>(energy) + SLEEP_ENERGY_GAIN);
    hunger = clampStat(static_cast<long long>(hunger) - SLEEP_HUNGER_COST);
    return ActionResult::success("Your pet slept well.");
}

ActionResult Pet::bath() {
    energy = clampStat(static_cast<long long>(energy) - BATH_ENERGY_COST);
    int gained = addHappiness(BATH_HAPPINESS_GAIN);
    return ActionResult::success("Squeaky clean!", gained);
}

void Pet::applyTimePassed(int ticks) {
    if (ticks <= 0) return;
    hunger = clampStat(static_cast<long long>(hunger) - static_cast<long long>(HUNGER_DECAY_PER_TICK) * ticks);
    energy = clampStat(static_cast<long long>(energy) - static_cast<long long>(ENERGY_DECAY_PER_TICK) * ticks);
}

void Pet::equip(DecorationSlot slot, int itemId) {
    equipment[slot] = itemId;
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
