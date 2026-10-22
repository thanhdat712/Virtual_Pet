#ifndef PET_H
#define PET_H

#include <string>
#include "../Item/Food.h"

// Lop co so truu tuong cho moi loai thu cung.
class Pet {
private:
    int id;
    std::string name;
    int level;
    int health;
    int happiness;
    int hunger; // 0 = no, 100 = rat doi
    int energy;

protected:
    static int clampStat(int value);
    void changeHealth(int amount);
    void changeHappiness(int amount);
    void changeHunger(int amount);
    void changeEnergy(int amount);

public:
    static constexpr int MIN_STAT = 0;
    static constexpr int MAX_STAT = 100;

    Pet(int id, const std::string& name, int level = 1,
        int health = 100, int happiness = 0, int hunger = 50, int energy = 50);
    virtual ~Pet() = default;

    int getId() const;
    const std::string& getName() const;
    int getLevel() const;
    int getHealth() const;
    int getHappiness() const;
    int getHunger() const;
    int getEnergy() const;

    void setName(const std::string& newName);
    void setHealth(int value);
    void setHappiness(int value);
    void setHunger(int value);
    void setEnergy(int value);

    // Food restore la do doi, nen an se lam hunger giam.
    bool eat(const Food& food);
    virtual void play();
    virtual void sleep();
    virtual void bath();

    // Goi theo khoang thoi gian (phut); chi so giam tang dan theo thoi gian.
    void updateOverTime(int minutes);
    virtual std::string getType() const = 0;
    virtual std::string getSound() const = 0;
};

#endif
