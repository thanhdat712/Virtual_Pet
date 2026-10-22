#include "Dog.h"

Dog::Dog(int id, const std::string& name, int level, int health, int happiness, int hunger, int energy)
    : Pet(id, name, level, health, happiness, hunger, energy) {}

std::string Dog::getType() const { return "Chó"; }
std::string Dog::getSound() const { return "Gâu gâu!"; }
void Dog::play() {
    if (getEnergy() < 15 || getHealth() == 0) return;
    setHappiness(getHappiness() + 18);
    setEnergy(getEnergy() - 15);
    setHunger(getHunger() + 12);
}
