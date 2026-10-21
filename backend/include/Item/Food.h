#ifndef FOOD_H
#define FOOD_H

#include "Item.h"

class Food : public Item {
private:
    int hungerRestore;    // ăn vào tăng chỉ số no thêm bao nhiêu %, thang 0-100
    int happinessRestore; // ăn vào tăng chỉ số vui thêm bao nhiêu %, thang 0-100, mặc định 0

public:
    Food(int id, const std::string& name, const std::string& description, int price,
         const std::string& imagePath, int hungerRestore, int happinessRestore = 0);

    int getHungerRestore() const;
    int getHappinessRestore() const;

    ItemType getType() const override;
};

#endif
