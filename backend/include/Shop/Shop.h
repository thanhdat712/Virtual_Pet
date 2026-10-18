#ifndef SHOP_H
#define SHOP_H

#include <vector>
#include <memory>
#include "../Item/Item.h"

class Shop {
private:
    std::vector<std::shared_ptr<Item>> itemsForSale;

public:
    Shop(); // tạo shop và tự bày sẵn danh sách món ăn mặc định lên kệ

    const std::vector<std::shared_ptr<Item>>& getItemsForSale() const;
};

#endif
