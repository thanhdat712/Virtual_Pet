#ifndef PET_H
#define PET_H

#include <map>
#include <string>

#include "../Core/ActionResult.h"
#include "../Item/Decoration.h" // DecorationSlot

class Food;

// Thú cưng của người chơi.
// Quy ước chỉ số (khớp frontend và bảng pets trong database):
//   - hunger là mức NO: 100 = no căng, 0 = đói; Food::hungerRestore làm giá trị này TĂNG.
//   - happiness vừa là độ vui vừa là thanh lên cấp: đầy 100 thì lên 1 cấp và thanh quay về phần dư.
// Các hành động là hàm virtual để Cat/Dog (KAN-31/32) ghi đè hành vi riêng.
class Pet {
private:
    int id;
    std::string name;
    int level;
    int health;
    int happiness;
    int hunger;
    int energy;
    std::map<DecorationSlot, int> equipment; // slot -> id đồ đang gắn (không có = chưa gắn)

protected:
    int addHappiness(int amount); // cộng/trừ độ vui, trả về số cấp vừa lên
    static int clampStat(int value);
    void changeHealth(int amount);
    void changeHappiness(int amount);
    void changeHunger(int amount);
    void changeEnergy(int amount);

public:
    static constexpr int MIN_STAT = 0;
    static constexpr int MAX_STAT = 100;

// Giá trị cân bằng game: chỉnh tại đây.
static constexpr int PLAY_HAPPINESS_GAIN = 15;
static constexpr int PLAY_ENERGY_COST = 15;
static constexpr int PLAY_HUNGER_COST = 10;
static constexpr int SLEEP_ENERGY_GAIN = 40;
static constexpr int SLEEP_HUNGER_COST = 5;
static constexpr int BATH_HAPPINESS_GAIN = 5;
static constexpr int BATH_ENERGY_COST = 5;
static constexpr int HUNGER_DECAY_PER_TICK = 1;
static constexpr int ENERGY_DECAY_PER_TICK = 1;

// Các chỉ số được kẹp trong khoảng [MIN_STAT, MAX_STAT];
// level tối thiểu là 1.
Pet(int id, const std::string& name,
    int level = 1,
    int health = 100,
    int happiness = 0,
    int hunger = 50,
    int energy = 50);
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

    // Hành động của pet
    virtual ActionResult feed(const Food& food);
    virtual ActionResult play();
    virtual ActionResult sleep();
    virtual ActionResult bath();

    // Tương thích với phần code cũ nếu các hàm này vẫn được sử dụng.
    bool eat(const Food& food);
    void updateOverTime(int minutes);
    virtual void applyTimePassed(int ticks);

    virtual std::string getType() const = 0;
    virtual std::string getSound() const = 0;

    // Đồ trang trí đang gắn
    void equip(DecorationSlot slot, int itemId);
    void unequip(DecorationSlot slot);
    int getEquipped(DecorationSlot slot) const;
    const std::map<DecorationSlot, int>& getEquipment() const;
};

#endif
