#ifndef PET_H
#define PET_H

#include <map>
#include <string>

#include "../Core/ActionResult.h"
#include "../Item/Decoration.h"

class Food;

class Pet {
private:
    int id;
    std::string name;
    int level;
    int health;
    int happiness;
    int hunger;
    int energy;
    int experience;
    std::map<DecorationSlot, int> equipment;

protected:
    int addHappiness(int amount);
    static int clampStat(int value);
    void changeHealth(int amount);
    void changeHappiness(int amount);
    void changeHunger(int amount);
    void changeEnergy(int amount);

public:
    static constexpr int MIN_STAT = 0;
    static constexpr int MAX_STAT = 100;

    static constexpr int PLAY_HAPPINESS_GAIN = 15;
    static constexpr int PLAY_ENERGY_COST = 15;
    static constexpr int PLAY_HUNGER_COST = 10;
    static constexpr int SLEEP_ENERGY_GAIN = 1;
    static constexpr int SLEEP_HUNGER_COST = 5;
    static constexpr int BATH_HAPPINESS_GAIN = 5;
    static constexpr int BATH_ENERGY_COST = 5;
    static constexpr int HUNGER_DECAY_PER_TICK = 1;
    static constexpr int ENERGY_DECAY_PER_TICK = 1;

    Pet(int id, const std::string& name,
        int level = 1,
        int health = 100,
        int happiness = 0,
        int hunger = 50,
        int energy = 50,
        int experience = 0);

    virtual ~Pet() = default;

    int getId() const;
    const std::string& getName() const;
    int getLevel() const;
    int getHealth() const;
    int getHappiness() const;
    int getHunger() const;
    int getEnergy() const;
    int getExperience() const;

    void setName(const std::string& newName);
    void setHealth(int value);
    void setHappiness(int value);
    void setHunger(int value);
    void setEnergy(int value);
    void setExperience(int value);

    virtual ActionResult feed(const Food& food);
    virtual ActionResult play();
    virtual ActionResult sleep();
    virtual ActionResult bath();

    bool eat(const Food& food);
    void updateOverTime(int minutes);
    virtual void applyTimePassed(int ticks);

    virtual std::string getType() const = 0;
    virtual std::string getSound() const = 0;

    void equip(DecorationSlot slot, int itemId);
    void unequip(DecorationSlot slot);
    int getEquipped(DecorationSlot slot) const;
    const std::map<DecorationSlot, int>& getEquipment() const;
};

#endif