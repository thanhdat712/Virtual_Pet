#ifndef ITEM_H
#define ITEM_H

#include <string>

// Phân loại item, dùng khi Inventory cần biết item nào thuộc loại nào
enum class ItemType {
    FOOD,
    DECORATION
};

class Item {
protected:
    int id;
    std::string name;
    std::string description;
    int price;
    std::string imagePath; // đường dẫn ảnh, frontend dùng để hiển thị, ví dụ "assets/items/apple.png"

public:
    Item(int id, const std::string& name, const std::string& description, int price, const std::string& imagePath);
    virtual ~Item();

    int getId() const;
    std::string getName() const;
    std::string getDescription() const;
    int getPrice() const;
    std::string getImagePath() const;

    // Food/Decoration bắt buộc phải tự trả lời mình là loại gì
    virtual ItemType getType() const = 0;
};

#endif
