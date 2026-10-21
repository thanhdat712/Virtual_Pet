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

    // Do trang tri khong cong don: moi mon chi so huu toi da 1 cai (co = 1, chua co = 0).
    if (item->getType() == ItemType::DECORATION) {
        if (index == -1) {
            slots.push_back({item, 1}); // chua co -> them dung 1 cai
        }
        return; // da co roi -> giu nguyen, bo qua so luong truyen vao
    }

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
