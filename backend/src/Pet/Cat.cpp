#include "Cat.h"

Cat::Cat(int id, const std::string& name, int level, int health, int happiness, int hunger, int energy)
    : Pet(id, name, level, health, happiness, hunger, energy) {}

std::string Cat::getType() const { return "Mèo"; }
std::string Cat::getSound() const { return "Meo meo!"; }
ActionResult Cat::play() {
    if (getHealth() <= 0 || getEnergy() < Pet::PLAY_ENERGY_COST) {
        return ActionResult::failure(ActionCode::PET_TOO_TIRED, "Your cat is too tired to play.");
    }

    changeEnergy(-Pet::PLAY_ENERGY_COST);
    changeHunger(-Pet::PLAY_HUNGER_COST);
    int gained = addHappiness(Pet::PLAY_HAPPINESS_GAIN);
    return ActionResult::success("Your cat had fun!", gained);
}
