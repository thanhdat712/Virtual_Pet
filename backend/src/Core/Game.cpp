#include "Game.h"
#include "PlayerGameData.h"
#include "Pet.h"
#include "Cat.h"
#include "Shop.h"
#include "Reward.h"
#include "Food.h"
#include "Decoration.h"
#include <set>
#include <string>

namespace {
const char* statusName(GameStatus status) {
    switch (status) {
        case GameStatus::NOT_STARTED: return "NOT_STARTED";
        case GameStatus::RUNNING:     return "RUNNING";
        case GameStatus::PAUSED:      return "PAUSED";
        case GameStatus::ENDED:       return "ENDED";
    }
    return "UNKNOWN";
}

ActionResult wrongState(const char* action, GameStatus current) {
    return ActionResult::failure(ActionCode::INVALID_STATE,
        std::string("Cannot ") + action + " while the game is " + statusName(current) + ".");
}

bool statInRange(int v) { return v >= Pet::MIN_STAT && v <= Pet::MAX_STAT; }
}

Game::Game() {
    // GameState đã tự khởi tạo trong constructor của nó
}

void Game::initialize() {
    gameState.initialize();
    timeCarrySeconds = 0;
}

// ---------------- Vòng đời ----------------

ActionResult Game::start() {
    if (gameState.getStatus() != GameStatus::NOT_STARTED) {
        return wrongState("start", gameState.getStatus());
    }
    gameState.setStatus(GameStatus::RUNNING);
    return ActionResult::success("Game started.");
}

ActionResult Game::pause() {
    if (gameState.getStatus() != GameStatus::RUNNING) {
        return wrongState("pause", gameState.getStatus());
    }
    gameState.setStatus(GameStatus::PAUSED);
    return ActionResult::success("Game paused.");
}

ActionResult Game::resume() {
    if (gameState.getStatus() != GameStatus::PAUSED) {
        return wrongState("resume", gameState.getStatus());
    }
    gameState.setStatus(GameStatus::RUNNING);
    return ActionResult::success("Game resumed.");
}

ActionResult Game::end() {
    GameStatus current = gameState.getStatus();
    if (current != GameStatus::RUNNING && current != GameStatus::PAUSED) {
        return wrongState("end", current);
    }
    gameState.setStatus(GameStatus::ENDED);
    return ActionResult::success("Game ended.");
}

GameStatus Game::getStatus() const {
    return gameState.getStatus();
}

// ---------------- Hành động ----------------

ActionResult Game::feed(int foodId) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("feed the pet", getStatus());

    auto item = gameState.getShop().getItemById(foodId);
    if (!item) {
        return ActionResult::failure(ActionCode::ITEM_NOT_FOUND, "Item not found.");
    }
    auto food = std::dynamic_pointer_cast<Food>(item);
    if (!food) {
        return ActionResult::failure(ActionCode::NOT_FOOD, "This item is not food.");
    }
    Inventory& bag = gameState.getPlayer().getInventory();
    if (bag.getQuantity(foodId) < 1) {
        return ActionResult::failure(ActionCode::NOT_OWNED, "No food remaining.");
    }

    // Đã kiểm tra hết -> mới đổi dữ liệu
    bag.removeItem(foodId, 1);
    return gameState.getPet().feed(*food);
}

ActionResult Game::play() {
    if (getStatus() != GameStatus::RUNNING) return wrongState("play", getStatus());
    return gameState.getPet().play();
}

ActionResult Game::sleep() {
    if (getStatus() != GameStatus::RUNNING) return wrongState("put the pet to sleep", getStatus());
    return gameState.getPet().sleep();
}

ActionResult Game::bath() {
    if (getStatus() != GameStatus::RUNNING) return wrongState("bathe the pet", getStatus());
    return gameState.getPet().bath();
}

ActionResult Game::buy(int itemId, int quantity) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("buy items", getStatus());
    return gameState.getShop().purchase(gameState.getPlayer(), itemId, quantity);
}

ActionResult Game::decorate(int itemId) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("decorate", getStatus());

    auto item = gameState.getShop().getItemById(itemId);
    if (!item) {
        return ActionResult::failure(ActionCode::ITEM_NOT_FOUND, "Item not found.");
    }
    auto deco = std::dynamic_pointer_cast<Decoration>(item);
    if (!deco) {
        return ActionResult::failure(ActionCode::NOT_DECORATION, "This item is not a decoration.");
    }
    if (!gameState.getPlayer().getInventory().hasItem(itemId)) {
        return ActionResult::failure(ActionCode::NOT_OWNED, "You do not own this decoration.");
    }
    gameState.getPet().equip(deco->getSlot(), itemId);
    return ActionResult::success(deco->getName() + " equipped.");
}

ActionResult Game::unequipDecoration(DecorationSlot slot) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("remove a decoration", getStatus());
    gameState.getPet().unequip(slot);
    return ActionResult::success("Decoration removed.");
}

ActionResult Game::claimReward(const Reward& reward) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("claim a reward", getStatus());
    reward.grantTo(gameState.getPlayer());
    return ActionResult::success("Reward claimed: +" + std::to_string(reward.getCoins()) + " coins.");
}

// ---------------- Thời gian ----------------

ActionResult Game::updateOverTime(int elapsedSeconds) {
    if (getStatus() != GameStatus::RUNNING) return wrongState("update the pet over time", getStatus());
    if (elapsedSeconds < 0) {
        return ActionResult::failure(ActionCode::INVALID_ARGUMENT, "Elapsed time cannot be negative.");
    }
    long long total = static_cast<long long>(timeCarrySeconds) + elapsedSeconds;
    long long ticks = total / TICK_SECONDS;
    timeCarrySeconds = static_cast<int>(total % TICK_SECONDS);
    if (ticks > 0) {
        int safeTicks = ticks > 1000000 ? 1000000 : static_cast<int>(ticks);
        gameState.getPet().applyTimePassed(safeTicks);
    }
    return ActionResult::success();
}

// ---------------- Lưu / nạp ----------------

GameSnapshot Game::exportSnapshot() const {
    GameSnapshot s;
    const PlayerData& user = gameState.getPlayer();
    const Pet& pet = gameState.getPet();

    s.userId = user.getId();
    s.userName = user.getName();
    s.coins = user.getCoins();

    s.pet.id = pet.getId();
    s.pet.name = pet.getName();
    s.pet.level = pet.getLevel();
    s.pet.health = pet.getHealth();
    s.pet.happiness = pet.getHappiness();
    s.pet.hunger = pet.getHunger();
    s.pet.energy = pet.getEnergy();

    for (const auto& slot : user.getInventory().getSlots()) {
        InventoryEntrySnapshot entry;
        entry.itemId = slot.item->getId();
        entry.quantity = slot.quantity;
        s.inventory.push_back(entry);
    }
    for (const auto& entry : pet.getEquipment()) {
        EquipmentEntrySnapshot eq;
        eq.slot = entry.first;
        eq.itemId = entry.second;
        s.equipment.push_back(eq);
    }
    return s;
}

ActionResult Game::importSnapshot(const GameSnapshot& s) {
    auto bad = [](const std::string& why) {
        return ActionResult::failure(ActionCode::INVALID_SNAPSHOT, "Invalid saved data: " + why);
    };

    if (s.coins < 0) return bad("coins is negative.");
    if (s.pet.level < 1) return bad("pet level must be at least 1.");
    if (!statInRange(s.pet.health) || !statInRange(s.pet.happiness) ||
        !statInRange(s.pet.hunger) || !statInRange(s.pet.energy)) {
        return bad("pet stats must be between 0 and 100.");
    }

    // Dựng đối tượng mới vào biến tạm, kiểm tra xong mới thay vào ván hiện tại
    auto user = std::make_unique<PlayerData>(s.userId, s.userName, s.coins);
    std::set<int> seen;
    for (const auto& entry : s.inventory) {
        auto item = gameState.getShop().getItemById(entry.itemId);
        if (!item) return bad("unknown item id " + std::to_string(entry.itemId) + ".");
        if (!seen.insert(entry.itemId).second) return bad("duplicate item id " + std::to_string(entry.itemId) + ".");
        if (entry.quantity < 1) return bad("item quantity must be at least 1.");
        if (item->getType() == ItemType::DECORATION && entry.quantity != 1) {
            return bad("a decoration can only be owned once.");
        }
        user->getInventory().addItem(item, entry.quantity);
    }

    auto pet = std::make_shared<Cat>(s.pet.id, s.pet.name, s.pet.level,
                                    s.pet.health, s.pet.happiness, s.pet.hunger, s.pet.energy);
    for (const auto& entry : s.equipment) {
        auto deco = std::dynamic_pointer_cast<Decoration>(gameState.getShop().getItemById(entry.itemId));
        if (!deco || deco->getSlot() != entry.slot) return bad("equipped item does not match its slot.");
        if (!user->getInventory().hasItem(entry.itemId)) return bad("equipped item is not in the inventory.");
        pet->equip(entry.slot, entry.itemId);
    }

    user->setPet(pet);
    gameState.restore(std::move(user), pet);
    timeCarrySeconds = 0;
    return ActionResult::success("Game data loaded.");
}

// ---------------- Truy cập ----------------

PlayerData& Game::getPlayer() { return gameState.getPlayer(); }
const PlayerData& Game::getPlayer() const { return gameState.getPlayer(); }
Pet& Game::getPet() { return gameState.getPet(); }
const Pet& Game::getPet() const { return gameState.getPet(); }
Shop& Game::getShop() { return gameState.getShop(); }
const Shop& Game::getShop() const { return gameState.getShop(); }
const Inventory& Game::getInventory() const { return gameState.getPlayer().getInventory(); }
int Game::getCoins() const { return gameState.getPlayer().getCoins(); }
