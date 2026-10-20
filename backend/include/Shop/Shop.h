#ifndef SHOP_H
#define SHOP_H

#include <vector>
#include <memory>
#include "../Item/Item.h"

class User; // khai bao truoc: mua hang can tru xu + them vao tui do cua User

class Shop {
private:
    std::vector<std::shared_ptr<Item>> itemsForSale;

public:
    Shop(); // tạo shop và tự bày sẵn danh sách món ăn mặc định lên kệ

    const std::vector<std::shared_ptr<Item>>& getItemsForSale() const;

    std::shared_ptr<Item> getItemById(int itemId) const; // tim mon theo id, nullptr neu khong co

    // Nguoi choi mua 1 mon theo id: tru xu va bo vao tui do (noi Shop -> Inventory).
    // Tra ve false neu khong tim thay mon hoac khong du xu.
    bool buyItem(User& user, int itemId);
};

#endif
