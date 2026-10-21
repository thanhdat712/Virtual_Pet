#ifndef PET_H
#define PET_H

#include <string>

// Thú cưng của người chơi.
// Đây là bản TỐI THIỂU cho KAN-36 (GAME-02) để GameState có pet mặc định.
// KAN-29/30 sẽ mở rộng (ăn/chơi/ngủ/tắm, giảm chỉ số theo thời gian),
// KAN-31/32 cho Cat/Dog kế thừa lớp này.
class Pet {
private:
    int id;
    std::string name;
    int health;
    int happiness;
    int hunger; // 0 = no, 100 = rất đói (Food::hungerRestore làm giá trị này giảm)
    int energy;

public:
    static constexpr int MIN_STAT = 0;
    static constexpr int MAX_STAT = 100;

    // Các chỉ số được kẹp trong khoảng [MIN_STAT, MAX_STAT]
    Pet(int id, const std::string& name,
        int health = 100, int happiness = 80, int hunger = 20, int energy = 100);
    virtual ~Pet() = default;

    int getId() const;
    const std::string& getName() const;
    int getHealth() const;
    int getHappiness() const;
    int getHunger() const;
    int getEnergy() const;
};

#endif
