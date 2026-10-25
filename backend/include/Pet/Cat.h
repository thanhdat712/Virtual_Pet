#ifndef CAT_H
#define CAT_H

#include "Pet.h"

class Cat : public Pet {
public:
    Cat(int id, const std::string& name, int level = 1,
        int health = 100, int happiness = 0, int hunger = 50, int energy = 50);
    std::string getType() const override;
    std::string getSound() const override;
    ActionResult play() override;
};

#endif
