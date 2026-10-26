#ifndef SHOP_H
#define SHOP_H

#include <vector>
#include <memory>
#include "../Item/Item.h"
#include "../Item/Decoration.h" // can DecorationSlot de loc do trang tri theo khu (hat/background/floor)
#include "../Core/ActionResult.h"

class PlayerData; // khai bao truoc: mua hang can tru xu + them vao tui do cua PlayerData

class Shop {
private:
    std::vector<std::shared_ptr<Item>> items;

public:
    Shop();

    const std::vector<std::shared_ptr<Item>>& getItems() const; // tat ca cac mon dang ban
    const std::vector<std::shared_ptr<Item>>& getItemsForSale() const { return getItems(); } // tương thích ngược
    std::vector<std::shared_ptr<Item>> getFoods() const;         // chi lay mon an
    std::vector<std::shared_ptr<Item>> getDecorations() const;   // chi lay do trang tri
    std::vector<std::shared_ptr<Item>> getDecorationsBySlot(DecorationSlot slot) const; // loc theo vi tri gan

    std::shared_ptr<Item> getItemById(int itemId) const; // tim mon theo id, nullptr neu khong co

    // Nguoi choi mua `quantity` cai cua 1 mon: tru xu va bo vao tui do (noi Shop -> Inventory).
    // Khong bao gio tru xu khi mua that bai. Ma loi: ITEM_NOT_FOUND, INVALID_ARGUMENT (so luong < 1,
    // hoac mua do trang tri > 1 cai), ALREADY_OWNED (do trang tri da co), NOT_ENOUGH_COINS.
    ActionResult purchase(PlayerData& player, int itemId, int quantity = 1);

    // Ban cu, giu de code cu van chay: mua 1 mon, chi tra ve thanh cong hay khong.
    bool buyItem(PlayerData& player, int itemId);
};

#endif
