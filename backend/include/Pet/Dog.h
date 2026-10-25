#ifndef DOG_H
#define DOG_H

#include "Pet.h"

class Dog : public Pet {
public:
    Dog(int id, const std::string& name, int level = 1,
        int health = 100, int happiness = 0, int hunger = 50, int energy = 50);
    std::string getType() const override;
    std::string getSound() const override;
    void play() override;
};

#endif
