// Test cho KAN-36 (GAME-02): khoi tao game state gom User, Pet, Shop.
#include <iostream>
#include "Game.h"
#include "User.h"
#include "Pet.h"
#include "Shop.h"

static int passed = 0;
static int failed = 0;

static void check(const std::string& ten, bool dieuKien) {
    if (dieuKien) {
        std::cout << "  [PASS] " << ten << "\n";
        passed++;
    } else {
        std::cout << "  [FAIL] " << ten << "\n";
        failed++;
    }
}

int main() {
    std::cout << "=== TEST GAME STATE (KAN-36) ===\n";

    // ----- Khoi tao mac dinh -----
    std::cout << "\n-- Khoi tao mac dinh --\n";
    Game game;
    check("Moi tao -> trang thai NOT_STARTED", game.getStatus() == GameStatus::NOT_STARTED);
    check("Nguoi choi mac dinh co id = 1", game.getPlayer().getId() == 1);
    check("Nguoi choi mac dinh co ten", !game.getPlayer().getName().empty());
    check("Xu khoi diem = 100", game.getPlayer().getCoins() == 100);
    check("Tui do ban dau rong", game.getPlayer().getInventory().isEmpty());
    check("Pet mac dinh co ten", !game.getPet().getName().empty());
    check("Chi so pet nam trong 0..100",
          game.getPet().getHealth() >= 0 && game.getPet().getHealth() <= 100 &&
          game.getPet().getHappiness() >= 0 && game.getPet().getHappiness() <= 100 &&
          game.getPet().getHunger() >= 0 && game.getPet().getHunger() <= 100 &&
          game.getPet().getEnergy() >= 0 && game.getPet().getEnergy() <= 100);
    check("Shop co san mon de ban", !game.getShop().getItemsForSale().empty());
    check("Shop co Banh mi (id 1)", game.getShop().getItemById(1) != nullptr);

    // ----- Pet: kep chi so -----
    std::cout << "\n-- Pet kep chi so --\n";
    Pet p(9, "Test", 150, -5, 200, 50);
    check("Health 150 -> 100", p.getHealth() == 100);
    check("Happiness -5 -> 0", p.getHappiness() == 0);
    check("Hunger 200 -> 100", p.getHunger() == 100);
    check("Energy 50 giu nguyen", p.getEnergy() == 50);

    // ----- Luong game van chay dung -----
    std::cout << "\n-- Chuyen trang thai --\n";
    game.start();
    check("start -> RUNNING", game.getStatus() == GameStatus::RUNNING);
    game.pause();
    check("pause -> PAUSED", game.getStatus() == GameStatus::PAUSED);
    game.resume();
    check("resume -> RUNNING", game.getStatus() == GameStatus::RUNNING);

    // ----- Noi cac module: mua do qua Game -----
    std::cout << "\n-- Mua do qua Game --\n";
    bool mua = game.getShop().buyItem(game.getPlayer(), 1); // Banh mi, gia 5
    check("Mua Banh mi thanh cong", mua);
    check("Xu giam 100 -> 95", game.getPlayer().getCoins() == 95);
    check("Tui co Banh mi", game.getPlayer().getInventory().hasItem(1));

    // ----- initialize() lam moi van game -----
    std::cout << "\n-- initialize() lam moi --\n";
    game.initialize();
    check("Sau initialize -> NOT_STARTED", game.getStatus() == GameStatus::NOT_STARTED);
    check("Sau initialize -> xu tro lai 100", game.getPlayer().getCoins() == 100);
    check("Sau initialize -> tui do rong", game.getPlayer().getInventory().isEmpty());

    // ----- Hai van game doc lap -----
    std::cout << "\n-- Hai van game doc lap --\n";
    Game a;
    Game b;
    a.getShop().buyItem(a.getPlayer(), 3); // Tao vang, gia 25
    check("Game a: xu = 75", a.getPlayer().getCoins() == 75);
    check("Game b khong bi anh huong: xu = 100", b.getPlayer().getCoins() == 100);

    std::cout << "\nKet qua: " << passed << " dat, " << failed << " that bai\n";
    return failed == 0 ? 0 : 1;
}
