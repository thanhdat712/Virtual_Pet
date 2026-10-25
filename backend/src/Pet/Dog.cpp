#include "Dog.h"

Dog::Dog(int id, const std::string& name, int level, int health, int happiness, int hunger, int energy)
    : Pet(id, name, level, health, happiness, hunger, energy) {}

std::string Dog::getType() const { return "Chó"; }
std::string Dog::getSound() const { return "Gâu gâu!"; }
ActionResult Dog::play() {
    if (getHealth() <= 0 || getEnergy() < Pet::PLAY_ENERGY_COST) {
        return ActionResult::failure(ActionCode::PET_TOO_TIRED, "Your dog is too tired to play.");
    }

    changeEnergy(-Pet::PLAY_ENERGY_COST);
    changeHunger(-Pet::PLAY_HUNGER_COST);
    int gained = addHappiness(Pet::PLAY_HAPPINESS_GAIN);
    return ActionResult::success("Your dog had fun!", gained);
}
