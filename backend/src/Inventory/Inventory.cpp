#include "Inventory.h"

Inventory::Inventory() {}

int Inventory::findSlotIndex(int itemId) const {
    for (size_t i = 0; i < slots.size(); i++) {
        if (slots[i].item->getId() == itemId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Inventory::addItem(std::shared_ptr<Item> item, int quantity) {
    int index = findSlotIndex(item->getId());
    if (index != -1) {
        slots[index].quantity += quantity;
    } else {
        slots.push_back({item, quantity});
    }
}

bool Inventory::removeItem(int itemId, int quantity) {
    int index = findSlotIndex(itemId);
    if (index == -1 || slots[index].quantity < quantity) {
        return false;
    }

    slots[index].quantity -= quantity;
    if (slots[index].quantity == 0) {
        slots.erase(slots.begin() + index);
    }
    return true;
}

int Inventory::getQuantity(int itemId) const {
    int index = findSlotIndex(itemId);
    return index == -1 ? 0 : slots[index].quantity;
}

bool Inventory::hasItem(int itemId) const {
    return findSlotIndex(itemId) != -1;
}

bool Inventory::isEmpty() const {
    return slots.empty();
}

const std::vector<InventorySlot>& Inventory::getSlots() const {
    return slots;
}
