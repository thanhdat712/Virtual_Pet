// Chuong trinh console de TEST TAY cac thao tac cua Inventory (KAN-46).
// Co ham main() rieng -> bien dich tach biet, khong gop chung voi main.cpp.
//
// Cach bien dich (chay trong thu muc backend):
//   g++ -std=c++17 -I include -I include/Inventory -I include/Item -I include/Shop -I include/User -I include/Reward ^
//       src/manual_test_inventory.cpp src/Inventory/Inventory.cpp src/Item/Item.cpp ^
//       src/Item/Food.cpp src/Item/Decoration.cpp src/Shop/Shop.cpp src/User/User.cpp src/Reward/Reward.cpp ^
//       -o manual_test_inventory.exe
//   ./manual_test_inventory.exe

#include <iostream>
#include <memory>
#include <vector>
#include <windows.h> // SetConsoleOutputCP de in tieng Viet co dau khong bi vo font
#include "Inventory.h"
#include "Shop.h"   // danh sach mon + buyItem (KAN-45)
#include "User.h"   // nguoi choi: co xu + tui do (KAN-44)
#include "Reward.h" // phan thuong: cong xu + item (KAN-45)
// In toan bo cac o dang co trong tui.
static void inTui(const Inventory& tui) {
    if (tui.isEmpty()) {
        cout << "  (Tui dang rong)\n";
        return;
    }
    std::cout << "  Tui dang co:\n";
    for (const auto& slot : tui.getSlots()) {
        std::cout << "    - [id " << slot.item->getId() << "] "
                  << slot.item->getName() << " x" << slot.quantity << "\n";
    }
}

// In danh sach mon co the chon.
static void inCatalog(const std::vector<std::shared_ptr<Item>>& catalog) {
    std::cout << "  Cac mon co the them (chon bang id):\n";
    for (const auto& item : catalog) {
        std::cout << "    id " << item->getId() << " = " << item->getName()
                  << " (gia " << item->getPrice() << " xu)\n";
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8); // bao console hieu chuoi UTF-8 -> tieng Viet co dau hien dung

    Shop shop;                       // Shop tu bay san 13 mon (do an + do trang tri)
    User nguoiChoi(1, "Ban", 100);   // nguoi choi co san 100 xu de mua do (KAN-44)
    std::vector<std::shared_ptr<Item>> catalog = shop.getItemsForSale();

    std::cout << "=== TEST TAY: USER + INVENTORY + SHOP + REWARD (KAN 41-45) ===\n";

    while (true) {
        std::cout << "\n---------------------------------\n";
        std::cout << "Xu hien co: " << nguoiChoi.getCoins() << "\n";
        std::cout << "1. Mua hang tu Shop (tru xu, bo vao tui)\n";
        std::cout << "2. Nhan thuong (cong xu + item vao tui)\n";
        std::cout << "3. Xoa item khoi tui\n";
        std::cout << "4. Xem so luong 1 item\n";
        std::cout << "5. Kiem tra tui co chua item khong\n";
        std::cout << "6. Kiem tra tui co rong khong\n";
        std::cout << "7. Xem toan bo tui\n";
        std::cout << "0. Thoat\n";
        std::cout << "Chon: ";

        int chon;
        if (!(std::cin >> chon)) {
            // Nguoi dung go chu thay vi so -> xoa trang thai loi va bo qua dong hong.
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "  Vui long nhap mot so!\n";
            continue;
        }

        if (chon == 0) {
            std::cout << "Tam biet!\n";
            break;
        }

        switch (chon) {
            case 1: { // Mua hang tu Shop -> tru xu + bo vao tui (KAN-44 + KAN-45)
                inCatalog(catalog);
                std::cout << "  Nhap id mon muon mua: ";
                int id;
                std::cin >> id;

                bool ok = shop.buyItem(nguoiChoi, id);
                if (ok) {
                    std::cout << "  Mua thanh cong! Xu con lai: " << nguoiChoi.getCoins() << "\n";
                } else {
                    std::cout << "  Mua that bai (khong co mon hoac khong du xu).\n";
                }
                inTui(nguoiChoi.getInventory());
                break;
            }
            case 2: { // Nhan mot phan thuong mau: +50 xu + Banh mi + Qua tao (KAN-45)
                Reward thuong(50);
                auto banhMi = shop.getItemById(1);
                auto quaTao = shop.getItemById(2);
                if (banhMi) thuong.addItem(banhMi);
                if (quaTao) thuong.addItem(quaTao);

                thuong.grantTo(nguoiChoi);
                std::cout << "  Da nhan thuong: +50 xu, +Banh mi, +Qua tao.\n";
                std::cout << "  Xu hien co: " << nguoiChoi.getCoins() << "\n";
                inTui(nguoiChoi.getInventory());
                break;
            }
            case 3: {
                std::cout << "  Nhap id mon can xoa: ";
                int id;
                std::cin >> id;
                std::cout << "  Nhap so luong can xoa: ";
                int sl;
                std::cin >> sl;

                bool ok = nguoiChoi.getInventory().removeItem(id, sl);
                if (ok) {
                    std::cout << "  Xoa thanh cong.\n";
                } else {
                    std::cout << "  Xoa that bai (khong co mon hoac khong du so luong).\n";
                }
                inTui(nguoiChoi.getInventory());
                break;
            }
            case 4: {
                std::cout << "  Nhap id mon: ";
                int id;
                std::cin >> id;
                std::cout << "  So luong hien co: " << nguoiChoi.getInventory().getQuantity(id) << "\n";
                break;
            }
            case 5: {
                std::cout << "  Nhap id mon: ";
                int id;
                std::cin >> id;
                std::cout << "  Tui " << (nguoiChoi.getInventory().hasItem(id) ? "CO" : "KHONG CO")
                          << " mon id = " << id << ".\n";
                break;
            }
            case 6: {
                std::cout << "  Tui " << (nguoiChoi.getInventory().isEmpty() ? "DANG RONG" : "KHONG RONG") << ".\n";
                break;
            }
            case 7: {
                inTui(nguoiChoi.getInventory());
                break;
            }
            default: {
                std::cout << "  Lua chon khong hop le.\n";
                break;
            }
        }
    }

    return 0;
}
