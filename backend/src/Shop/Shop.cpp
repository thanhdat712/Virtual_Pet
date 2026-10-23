#include "Shop.h"
#include "include/Item/Food.h"
#include "include/Item/Decoration.h"
#include "include/User/PlayerGameData.h"
#include <string>

Shop::Shop() {
    // --- Món ăn (Food) ---
    items.push_back(std::make_shared<Food>(
        1, "Bánh mì", "Món ăn no lâu, giá rẻ", 5, "assets/items/banh_mi.png", 30, 0));
    items.push_back(std::make_shared<Food>(
        2, "Quả táo", "Trái cây tươi mát", 8, "assets/items/tao_do.png", 20, 5));
    items.push_back(std::make_shared<Food>(
        3, "Táo vàng", "Táo quý hiếm, hồi phục mạnh", 25, "assets/items/tao_vang.png", 40, 15));
    items.push_back(std::make_shared<Food>(
        4, "Bánh kem", "Ngọt mềm, tăng vui nhiều", 6, "assets/items/banh_kem.png", 10, 20));
    items.push_back(std::make_shared<Food>(
        5, "Mật ong", "Ngọt tự nhiên, bổ dưỡng", 3, "assets/items/mat_ong.png", 5, 25));

    // --- Đồ trang trí đội đầu (HAT) ---
    items.push_back(std::make_shared<Decoration>(
        6, "Mũ len", "Mũ len ấm áp cho pet", 15, "assets/items/mu_len.png", DecorationSlot::HAT));
    items.push_back(std::make_shared<Decoration>(
        7, "Vương miện", "Vương miện sang trọng", 50, "assets/items/vuong_mien.png", DecorationSlot::HAT));

    // --- Hình nền (BACKGROUND) ---
    items.push_back(std::make_shared<Decoration>(
        8, "Nền bãi biển", "Khung cảnh biển xanh cát trắng", 20, "assets/items/nen_bai_bien.png", DecorationSlot::BACKGROUND));
    items.push_back(std::make_shared<Decoration>(
        9, "Nền bầu trời", "Bầu trời đầy sao lãng mạn", 30, "assets/items/nen_bau_troi.png", DecorationSlot::BACKGROUND));
    items.push_back(std::make_shared<Decoration>(
        12, "Nền khu rừng", "Rừng cây xanh mát yên bình", 25, "assets/items/nen_khu_rung.png", DecorationSlot::BACKGROUND));
    items.push_back(std::make_shared<Decoration>(
        13, "Nền thành phố", "Phố xá về đêm rực rỡ ánh đèn", 35, "assets/items/nen_thanh_pho.png", DecorationSlot::BACKGROUND));

    // --- Sàn nhà (FLOOR) ---
    items.push_back(std::make_shared<Decoration>(
        10, "Sàn gỗ", "Sàn gỗ ấm cúng", 18, "assets/items/san_go.png", DecorationSlot::FLOOR));
    items.push_back(std::make_shared<Decoration>(
        11, "Thảm hoa", "Thảm hoa mềm mại", 22, "assets/items/tham_hoa.png", DecorationSlot::FLOOR));
}

const std::vector<std::shared_ptr<Item>>& Shop::getItemsForSale() const {
    return items;
}

std::vector<std::shared_ptr<Item>> Shop::getFoods() const {
    std::vector<std::shared_ptr<Item>> result;
    for (const auto& item : items) {
        if (item->getType() == ItemType::FOOD) {
            result.push_back(item);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Item>> Shop::getDecorationsBySlot(DecorationSlot slot) const {
    std::vector<std::shared_ptr<Item>> result;
    for (const auto& item : items) {
        if (item->getType() != ItemType::DECORATION) {
            continue; // bo qua do an, chi xet trang tri
        }
        // ep kieu tu Item xuong Decoration de doc duoc slot (hat/background/floor)
        auto deco = std::dynamic_pointer_cast<Decoration>(item);
        if (deco && deco->getSlot() == slot) {
            result.push_back(item);
        }
    }
    return result;
}

std::shared_ptr<Item> Shop::getItemById(int itemId) const {
    for (const auto& item : items) {
        if (item->getId() == itemId) {
            return item;
        }
    }
    return nullptr;
}

ActionResult Shop::purchase(PlayerData& user, int itemId, int quantity) {
    if (quantity < 1) {
        return ActionResult::failure(ActionCode::INVALID_ARGUMENT, "Choose a valid quantity.");
    }
    std::shared_ptr<Item> item = getItemById(itemId);
    if (!item) {
        return ActionResult::failure(ActionCode::ITEM_NOT_FOUND, "Item not found.");
    }
    if (item->getType() == ItemType::DECORATION) {
        if (quantity != 1) {
            return ActionResult::failure(ActionCode::INVALID_ARGUMENT, "Choose a valid quantity.");
        }
        if (user.getInventory().hasItem(itemId)) {
            return ActionResult::failure(ActionCode::ALREADY_OWNED, "You already own this decoration.");
        }
    }
    long long total = static_cast<long long>(item->getPrice()) * quantity; // long long de khong tran so
    if (total > user.getCoins() || !user.spendCoins(static_cast<int>(total))) {
        return ActionResult::failure(ActionCode::NOT_ENOUGH_COINS, "Not enough coins for this quantity.");
    }
    user.getInventory().addItem(item, quantity); // mua thanh cong -> bo vao tui
    return ActionResult::success(std::to_string(quantity) + " x " + item->getName() + " added to your inventory.");
}

bool Shop::buyItem(PlayerData& user, int itemId) {
    return purchase(user, itemId, 1).ok();
}
