// Test cho KAN-45: noi Inventory voi Reward va Shop (KAN-46 mo rong).
// Bien dich rieng, KHONG gop chung voi main.cpp (vi co ham main() rieng).
#include <iostream>
#include <memory>
#include "../include/User/User.h"
#include "../include/Shop/Shop.h"
#include "../include/Reward/Reward.h"
#include "../include/Item/Food.h"

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
    std::cout << "=== TEST REWARD + SHOP -> INVENTORY (KAN-45) ===\n";

    // ----- Phan 1: Reward trao thuong vao tui do -----
    User u1(1, "An", 50);

    Reward r(100); // thuong 100 xu
    auto banhMi = std::make_shared<Food>(1, "Banh mi", "No lau", 5, "assets/banhmi.png", 30);
    auto quaTao = std::make_shared<Food>(2, "Qua tao", "Tuoi mat", 8, "assets/tao.png", 20);
    r.addItem(banhMi);
    r.addItem(quaTao);

    check("Reward giu dung so xu (getCoins = 100)", r.getCoins() == 100);
    check("Reward giu dung so item (getItems = 2)", r.getItems().size() == 2);

    r.grantTo(u1);
    check("Sau khi nhan thuong -> xu = 50 + 100 = 150", u1.getCoins() == 150);
    check("Nhan thuong -> tui co Banh mi", u1.getInventory().hasItem(1));
    check("Nhan thuong -> tui co Qua tao", u1.getInventory().hasItem(2));
    check("Nhan thuong -> moi mon 1 cai", u1.getInventory().getQuantity(1) == 1);

    // Nhan them lan nua -> gop so luong
    r.grantTo(u1);
    check("Nhan thuong lan 2 -> xu = 250", u1.getCoins() == 250);
    check("Nhan thuong lan 2 -> Banh mi gop thanh 2", u1.getInventory().getQuantity(1) == 2);

    // ----- Phan 2: Shop ban hang, tru xu va bo vao tui -----
    User u2(2, "Binh", 30);
    Shop shop;

    // getItemById
    check("Shop tim thay mon id = 1 (Banh mi)", shop.getItemById(1) != nullptr);
    check("Shop khong tim thay mon id = 999", shop.getItemById(999) == nullptr);

    // Mua Banh mi (id 1, gia 5) -> du xu
    int giaBanhMi = shop.getItemById(1)->getPrice();
    bool muaOk = shop.buyItem(u2, 1);
    check("Mua Banh mi -> tra ve true", muaOk);
    check("Mua xong -> xu bi tru dung gia", u2.getCoins() == 30 - giaBanhMi);
    check("Mua xong -> tui co Banh mi (id 1)", u2.getInventory().hasItem(1));
    check("Mua xong -> so luong Banh mi = 1", u2.getInventory().getQuantity(1) == 1);

    // Mua mon khong ton tai -> false, khong tru xu
    int xuTruocKhiMuaLoi = u2.getCoins();
    bool muaLoi = shop.buyItem(u2, 999);
    check("Mua mon khong co trong shop -> tra ve false", !muaLoi);
    check("Mua that bai -> xu giu nguyen", u2.getCoins() == xuTruocKhiMuaLoi);

    // Mua mon qua dat (Vuong mien id 7 gia 50) khi khong du xu -> false
    bool muaDat = shop.buyItem(u2, 7);
    check("Mua mon qua dat (khong du xu) -> tra ve false", !muaDat);
    check("Khong du xu -> khong co Vuong mien trong tui", !u2.getInventory().hasItem(7));

    std::cout << "\n=== KET QUA: " << passed << " PASS, " << failed << " FAIL ===\n";
    return failed == 0 ? 0 : 1;
}
