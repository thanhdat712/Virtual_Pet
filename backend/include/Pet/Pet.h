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

    int addHappiness(int amount); // cộng/trừ độ vui, trả về số cấp vừa lên

public:
    static constexpr int MIN_STAT = 0;
    static constexpr int MAX_STAT = 100;

    // Giá trị cân bằng game: chỉnh tại đây. (Chưa có số liệu chính thức cho play/sleep/bath/giảm theo thời gian.)
    static constexpr int PLAY_HAPPINESS_GAIN = 15;
    static constexpr int PLAY_ENERGY_COST = 15;
    static constexpr int PLAY_HUNGER_COST = 10;
    static constexpr int SLEEP_ENERGY_GAIN = 40;
    static constexpr int SLEEP_HUNGER_COST = 5;
    static constexpr int BATH_HAPPINESS_GAIN = 5;
    static constexpr int BATH_ENERGY_COST = 5;
    static constexpr int HUNGER_DECAY_PER_TICK = 1;
    static constexpr int ENERGY_DECAY_PER_TICK = 1;

    // Các chỉ số được kẹp trong khoảng [MIN_STAT, MAX_STAT]; level tối thiểu là 1.
    // Mặc định khớp bảng pets: happiness 85, hunger 30, energy 70.
    Pet(int id, const std::string& name,
        int health = 100, int happiness = 85, int hunger = 30, int energy = 70, int level = 1);
    virtual ~Pet() = default;

    int getId() const;
    const std::string& getName() const;
    int getLevel() const;
    int getHealth() const;
    int getHappiness() const;
    int getHunger() const;
    int getEnergy() const;

    // Hành động của pet. Trả về lỗi (không im lặng) nếu pet không làm được.
    virtual ActionResult feed(const Food& food); // ăn: tăng no, tăng vui (có thể lên cấp)
    virtual ActionResult play();                 // chơi: tăng vui, tốn năng lượng và no
    virtual ActionResult sleep();                // ngủ: hồi năng lượng
    virtual ActionResult bath();                 // tắm: tăng vui một chút

    // Giảm chỉ số khi thời gian trôi: mỗi "tick" làm pet đói và mệt hơn.
    virtual void applyTimePassed(int ticks);

    // Đồ trang trí đang gắn
    void equip(DecorationSlot slot, int itemId);
    int getEquipped(DecorationSlot slot) const; // 0 nếu chưa gắn gì
    const std::map<DecorationSlot, int>& getEquipment() const;
};

#endif
