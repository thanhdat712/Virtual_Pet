#include "Inventory.h"
#include <algorithm>

namespace {
int findSlotIndexInVector(const std::vector<InventorySlot>& slots, int itemId) {
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].item && slots[i].item->getId() == itemId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
}

Inventory::Inventory() = default;

int Inventory::findSlotIndex(int itemId) const {
    return findSlotIndexInVector(slots, itemId);
}

void Inventory::addItem(std::shared_ptr<Item> item, int quantity) {
    if (!item || quantity <= 0) {
        return;
    }

    const int index = findSlotIndex(item->getId());
    if (index >= 0) {
        slots[index].quantity += quantity;
        return;
    }

    slots.push_back(InventorySlot{item, quantity});
}

bool Inventory::removeItem(int itemId, int quantity) {
    if (quantity <= 0) {
        return false;
    }

    const int index = findSlotIndex(itemId);
    if (index < 0) {
        return false;
    }

    if (slots[index].quantity < quantity) {
        return false;
    }

    slots[index].quantity -= quantity;
    if (slots[index].quantity == 0) {
        slots.erase(slots.begin() + index);
    }
    return true;
}

int Inventory::getQuantity(int itemId) const {
    const int index = findSlotIndex(itemId);
    return index >= 0 ? slots[index].quantity : 0;
}

bool Inventory::hasItem(int itemId) const {
    return getQuantity(itemId) > 0;
}

bool Inventory::isEmpty() const {
    return slots.empty();
}

const std::vector<InventorySlot>& Inventory::getSlots() const {
    return slots;
}
