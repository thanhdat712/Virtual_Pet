#ifndef DECORATION_H
#define DECORATION_H

#include "Item.h"

// Vị trí gắn trang trí lên pet, chờ đồng bộ với bạn làm KAN-9 (Pet)
enum class DecorationSlot {
    HAT,
    BACKGROUND,
    FLOOR
};

class Decoration : public Item {
private:
    DecorationSlot slot;

public:
    Decoration(int id, const std::string& name, const std::string& description, int price,
               const std::string& imagePath, DecorationSlot slot);

    DecorationSlot getSlot() const;

    ItemType getType() const override;
};

#endif
