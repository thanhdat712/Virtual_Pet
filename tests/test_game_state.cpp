// Test cho KAN-36 (GAME-02): khoi tao game state gom User, Pet, Shop.
#include <iostream>
#include "Game.h"
#include "User.h"
#include "Pet.h"
#include "Cat.h"
#include "Dog.h"
#include "Food.h"
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
    check("Nang luong mac dinh = 50", game.getPet().getEnergy() == 50);
    check("Do doi mac dinh = 50", game.getPet().getHunger() == 50);
    check("Do vui mac dinh = 0", game.getPet().getHappiness() == 0);
    check("User so huu cung pet voi GameState", game.getPlayer().getPet().get() == &game.getPet());
    check("Chi so pet nam trong 0..100",
          game.getPet().getHealth() >= 0 && game.getPet().getHealth() <= 100 &&
          game.getPet().getHappiness() >= 0 && game.getPet().getHappiness() <= 100 &&
          game.getPet().getHunger() >= 0 && game.getPet().getHunger() <= 100 &&
          game.getPet().getEnergy() >= 0 && game.getPet().getEnergy() <= 100);
    check("Shop co san mon de ban", !game.getShop().getItemsForSale().empty());
    check("Shop co Banh mi (id 1)", game.getShop().getItemById(1) != nullptr);

    // ----- Pet: kep chi so -----
    std::cout << "\n-- Pet kep chi so --\n";
    Cat p(9, "Test", 1, 150, -5, 200, 50);
    check("Health 150 -> 100", p.getHealth() == 100);
    check("Happiness -5 -> 0", p.getHappiness() == 0);
    check("Hunger 200 -> 100", p.getHunger() == 100);
    check("Energy 50 giu nguyen", p.getEnergy() == 50);

    // ----- Setter luon kep chi so trong 0..100 -----
    std::cout << "\n-- Setter kep chi so --\n";
    p.setHealth(150);
    p.setHappiness(-10);
    p.setHunger(101);
    p.setEnergy(-1);
    check("setHealth(150) -> 100", p.getHealth() == 100);
    check("setHappiness(-10) -> 0", p.getHappiness() == 0);
    check("setHunger(101) -> 100", p.getHunger() == 100);
    check("setEnergy(-1) -> 0", p.getEnergy() == 0);

    // ----- Cho an truc tiep -----
    std::cout << "\n-- Pet an Food --\n";
    Cat petAn(10, "Mun", 1, 100, 20, 60, 50);
    Food banhMi(90, "Banh mi", "Thuc an test", 5, "assets/banhmi.png", 15, 5);
    check("Food co muc hoi phuc doi = 15", banhMi.getHungerRestore() == 15);
    check("Pet an thanh cong", petAn.eat(banhMi));
    check("An xong do doi giam 60 -> 45", petAn.getHunger() == 45);
    check("An xong do vui tang 20 -> 25", petAn.getHappiness() == 25);

    // ----- Da hinh qua con tro Pet* -----
    std::cout << "\n-- Da hinh Pet* --\n";
    Cat meoDaHinh(11, "Mi", 1, 100, 20, 30, 50);
    Dog choDaHinh(12, "Lu", 1, 100, 20, 30, 50);
    Pet* petPtr = &meoDaHinh;
    check("Pet* tro den Cat -> getType la Meo", petPtr->getType() == "Mèo");
    petPtr->play();
    check("Goi play qua Pet* thuc thi Cat::play", meoDaHinh.getHappiness() == 32 && meoDaHinh.getEnergy() == 42);
    petPtr = &choDaHinh;
    check("Pet* tro den Dog -> getType la Cho", petPtr->getType() == "Chó");
    petPtr->play();
    check("Goi play qua Pet* thuc thi Dog::play", choDaHinh.getHappiness() == 38 && choDaHinh.getEnergy() == 35);

    // ----- Chi so thay doi theo thoi gian -----
    std::cout << "\n-- Cap nhat chi so theo thoi gian --\n";
    Cat petTheoGio(13, "Theo", 1, 100, 40, 20, 70);
    petTheoGio.updateOverTime(30);
    check("Sau 30 phut do doi tang 3", petTheoGio.getHunger() == 23);
    check("Sau 30 phut nang luong giam 3", petTheoGio.getEnergy() == 67);
    check("Sau 30 phut do vui giam 3", petTheoGio.getHappiness() == 37);
    petTheoGio.updateOverTime(0);
    check("Thoi gian 0 khong doi chi so", petTheoGio.getHunger() == 23 &&
          petTheoGio.getEnergy() == 67 && petTheoGio.getHappiness() == 37);

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
    int hungerBeforeFeed = game.getPet().getHunger();
    check("Cho pet an thanh cong", game.getPlayer().feedPet(1));
    check("An xong do doi giam", game.getPet().getHunger() < hungerBeforeFeed);
    check("An xong banh mi bi tru kho", !game.getPlayer().getInventory().hasItem(1));

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
