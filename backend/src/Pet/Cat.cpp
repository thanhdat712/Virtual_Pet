#include "Cat.h"

Cat::Cat(int id, const std::string& name, int level, int health, int happiness, int hunger, int energy)
    : Pet(id, name, level, health, happiness, hunger, energy) {}

std::string Cat::getType() const { return "Mèo"; }
std::string Cat::getSound() const { return "Meo meo!"; }
void Cat::play() {
    if (getEnergy() < 10 || getHealth() == 0) return;
    setHappiness(getHappiness() + 12);
    setEnergy(getEnergy() - 8);
    setHunger(getHunger() + 8);
}
