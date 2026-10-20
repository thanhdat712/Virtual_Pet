#include "Decoration.h"

Decoration::Decoration(int id, const std::string& name, const std::string& description, int price,
                        const std::string& imagePath, DecorationSlot slot)
    : Item(id, name, description, price, imagePath), slot(slot) {}

DecorationSlot Decoration::getSlot() const {
    return slot;
}

ItemType Decoration::getType() const {
    return ItemType::DECORATION;
}
