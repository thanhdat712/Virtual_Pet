#ifndef SHOP_H
#define SHOP_H

#include <vector>
#include <memory>
#include "../Item/Item.h"
#include "../Item/Decoration.h" // can DecorationSlot de loc do trang tri theo khu (hat/background/floor)

class PlayerData;

class Shop {
private:
    std::vector<std::shared_ptr<Item>> itemsForSale;

public:
    Shop(); // tạo shop và tự bày sẵn danh sách món ăn mặc định lên kệ

    const std::vector<std::shared_ptr<Item>>& getItemsForSale() const;

    // Lay rieng tung nhom hang de hien thi theo khu vuc trong shop (KAN-47)
    std::vector<std::shared_ptr<Item>> getFoods() const;                        // chi lay do an
    std::vector<std::shared_ptr<Item>> getDecorationsBySlot(DecorationSlot slot) const; // lay trang tri theo loai

    std::shared_ptr<Item> getItemById(int itemId) const; // tim mon theo id, nullptr neu khong co

    // Nguoi choi mua 1 mon theo id: tru xu va bo vao tui do (noi Shop -> Inventory).
    // Tra ve false neu khong tim thay mon hoac khong du xu.
    bool buyItem(PlayerData& player, int itemId);
};

#endif
