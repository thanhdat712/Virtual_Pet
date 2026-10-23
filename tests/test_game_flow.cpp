// Test cho GAME-03/04/05/06: Game là điểm truy cập duy nhất, hành động trả mã lỗi,
// luồng game trọn vẹn, chuyển trạng thái sai, giảm chỉ số theo thời gian, lưu/nạp snapshot.
#include <iostream>
#include <string>
#include "Game.h"
#include "User.h"
#include "Pet.h"
#include "Shop.h"
#include "Reward.h"

static int passed = 0;
static int failed = 0;

static void check(const std::string& ten, bool dieuKien) {
    std::cout << (dieuKien ? "  [PASS] " : "  [FAIL] ") << ten << "\n";
    dieuKien ? passed++ : failed++;
}
static bool is(const ActionResult& r, ActionCode c) { return r.code == c; }

int main() {
    std::cout << "=== TEST GAME FLOW (GAME-03..06) ===\n";

    std::cout << "\n-- Chuyen trang thai hop le va sai --\n";
    {
        Game g;
        check("resume khi chua pause -> INVALID_STATE", is(g.resume(), ActionCode::INVALID_STATE));
        check("pause khi chua start -> INVALID_STATE", is(g.pause(), ActionCode::INVALID_STATE));
        check("end khi chua start -> INVALID_STATE", is(g.end(), ActionCode::INVALID_STATE));
        check("sai trang thai thi khong doi trang thai", g.getStatus() == GameStatus::NOT_STARTED);
        check("start OK", g.start().ok() && g.getStatus() == GameStatus::RUNNING);
        check("start lan 2 -> INVALID_STATE", is(g.start(), ActionCode::INVALID_STATE));
        check("resume khi dang RUNNING -> INVALID_STATE", is(g.resume(), ActionCode::INVALID_STATE));
        check("pause OK", g.pause().ok() && g.getStatus() == GameStatus::PAUSED);
        check("pause lan 2 -> INVALID_STATE", is(g.pause(), ActionCode::INVALID_STATE));
        check("resume OK", g.resume().ok() && g.getStatus() == GameStatus::RUNNING);
        check("end OK", g.end().ok() && g.getStatus() == GameStatus::ENDED);
        check("start khi ENDED -> INVALID_STATE", is(g.start(), ActionCode::INVALID_STATE));
        g.initialize();
        check("initialize dua ve NOT_STARTED", g.getStatus() == GameStatus::NOT_STARTED);
    }

    std::cout << "\n-- Hanh dong bi chan khi game khong RUNNING --\n";
    {
        Game g;
        check("buy khi NOT_STARTED -> INVALID_STATE", is(g.buy(1), ActionCode::INVALID_STATE));
        check("buy bi chan thi khong tru xu", g.getCoins() == 100);
        g.start(); g.pause();
        check("feed khi PAUSED -> INVALID_STATE", is(g.feed(1), ActionCode::INVALID_STATE));
        check("play khi PAUSED -> INVALID_STATE", is(g.play(), ActionCode::INVALID_STATE));
        check("sleep khi PAUSED -> INVALID_STATE", is(g.sleep(), ActionCode::INVALID_STATE));
        check("bath khi PAUSED -> INVALID_STATE", is(g.bath(), ActionCode::INVALID_STATE));
        check("decorate khi PAUSED -> INVALID_STATE", is(g.decorate(6), ActionCode::INVALID_STATE));
        check("updateOverTime khi PAUSED -> INVALID_STATE", is(g.updateOverTime(600), ActionCode::INVALID_STATE));
        check("pause thi pet khong doi", g.getPet().getHunger() == 30 && g.getPet().getEnergy() == 70);
    }

    std::cout << "\n-- Kich ban tron ven: khoi tao, mua, cho an, nhan thuong --\n";
    {
        Game g;
        g.start();
        check("Xu khoi diem 100", g.getCoins() == 100);

        ActionResult r = g.buy(1, 3); // Banh mi gia 5 x3 = 15
        check("mua 3 Banh mi OK", r.ok());
        check("xu 100 -> 85", g.getCoins() == 85);
        check("tui co 3 Banh mi", g.getInventory().getQuantity(1) == 3);

        check("mua so luong 0 -> INVALID_ARGUMENT", is(g.buy(1, 0), ActionCode::INVALID_ARGUMENT));
        check("mua mon khong ton tai -> ITEM_NOT_FOUND", is(g.buy(999), ActionCode::ITEM_NOT_FOUND));
        Game poor; poor.start();
        check("mua 3 do trang tri -> INVALID_ARGUMENT", is(poor.buy(7, 3), ActionCode::INVALID_ARGUMENT));
        check("mua 2 do trang tri -> INVALID_ARGUMENT, khong tru xu", poor.getCoins() == 100);
        check("mua that bai thi xu giu nguyen (Banh mi x30)", is(poor.buy(1, 30), ActionCode::NOT_ENOUGH_COINS) && poor.getCoins() == 100);

        // Cho an: hunger 30 + 30 = 60
        int hungerTruoc = g.getPet().getHunger();
        r = g.feed(1);
        check("feed Banh mi OK", r.ok());
        check("hunger tang 30 (no hon)", g.getPet().getHunger() == hungerTruoc + 30);
        check("tui con 2 Banh mi", g.getInventory().getQuantity(1) == 2);
        check("feed mon khong co trong tui -> NOT_OWNED", is(g.feed(2), ActionCode::NOT_OWNED));
        check("feed do trang tri -> NOT_FOOD", is(g.feed(6), ActionCode::NOT_FOOD));
        check("feed id khong ton tai -> ITEM_NOT_FOUND", is(g.feed(999), ActionCode::ITEM_NOT_FOUND));

        // Nhan thuong
        Reward thuong(50);
        thuong.addItem(g.getShop().getItemById(2)); // Qua tao
        check("claimReward OK", g.claimReward(thuong).ok());
        check("xu 85 + 50 = 135", g.getCoins() == 135);
        check("tui co Qua tao", g.getInventory().hasItem(2));
    }

    std::cout << "\n-- Len cap khi thanh vui day --\n";
    {
        Game g;
        g.start();
        g.buy(4, 1); // Banh kem: +10 no, +20 vui
        ActionResult r = g.feed(4);
        check("vui 85 + 20 = 105 -> len 1 cap", r.levelsGained == 1);
        check("cap pet = 2", g.getPet().getLevel() == 2);
        check("thanh vui giu phan du = 5", g.getPet().getHappiness() == 5);
    }

    std::cout << "\n-- Mua va gan do trang tri --\n";
    {
        Game g;
        g.start();
        check("decorate khi chua mua -> NOT_OWNED", is(g.decorate(6), ActionCode::NOT_OWNED));
        check("mua Mu len OK", g.buy(6).ok());
        check("xu 100 - 15 = 85", g.getCoins() == 85);
        check("mua lai do da co -> ALREADY_OWNED", is(g.buy(6), ActionCode::ALREADY_OWNED));
        check("mua lai bi chan thi KHONG tru xu", g.getCoins() == 85);
        check("decorate Mu len OK", g.decorate(6).ok());
        check("Mu len gan vao slot HAT", g.getPet().getEquipped(DecorationSlot::HAT) == 6);
        check("decorate do an -> NOT_DECORATION", is(g.decorate(1), ActionCode::NOT_DECORATION));
        check("decorate id khong ton tai -> ITEM_NOT_FOUND", is(g.decorate(999), ActionCode::ITEM_NOT_FOUND));
        g.buy(7);
        g.decorate(7);
        check("gan Vuong mien thay Mu len (cung slot)", g.getPet().getEquipped(DecorationSlot::HAT) == 7);
    }

    std::cout << "\n-- Play / sleep / bath --\n";
    {
        Game g;
        g.start();
        int vui = g.getPet().getHappiness();
        check("play OK", g.play().ok());
        check("play tang vui / len cap", g.getPet().getHappiness() != vui || g.getPet().getLevel() > 1);
        check("play ton nang luong", g.getPet().getEnergy() == 70 - Pet::PLAY_ENERGY_COST);
        check("sleep OK", g.sleep().ok());
        check("sleep hoi nang luong", g.getPet().getEnergy() == 70 - Pet::PLAY_ENERGY_COST + Pet::SLEEP_ENERGY_GAIN);
        g.sleep(); g.sleep();
        check("nang luong day -> sleep bao ENERGY_FULL", is(g.sleep(), ActionCode::ENERGY_FULL));
        check("bath OK", g.bath().ok());

        Game tired; tired.start();
        tired.updateOverTime(60 * 100); // 100 tick -> nang luong ve 0
        check("het nang luong", tired.getPet().getEnergy() == 0);
        check("play khi het nang luong -> PET_TOO_TIRED", is(tired.play(), ActionCode::PET_TOO_TIRED));
    }

    std::cout << "\n-- Cap nhat chi so theo thoi gian --\n";
    {
        Game g;
        g.start();
        g.updateOverTime(59);
        check("59 giay chua du 1 tick -> chi so khong doi", g.getPet().getHunger() == 30);
        g.updateOverTime(1);
        check("cong don du 60 giay -> hunger giam 1", g.getPet().getHunger() == 29);
        g.updateOverTime(150); // 2 tick, du 30 giay
        check("150 giay -> giam them 2 (tong 3)", g.getPet().getHunger() == 27);
        check("nang luong cung giam 3", g.getPet().getEnergy() == 67);
        check("so giay am -> INVALID_ARGUMENT", is(g.updateOverTime(-5), ActionCode::INVALID_ARGUMENT));
        g.updateOverTime(60 * 10000);
        check("chi so khong xuong duoi 0", g.getPet().getHunger() == 0 && g.getPet().getEnergy() == 0);
    }

    std::cout << "\n-- Luu / nap snapshot --\n";
    {
        Game g;
        g.start();
        g.buy(3, 2); g.buy(7); g.decorate(7); g.feed(3); g.updateOverTime(300);
        GameSnapshot snap = g.exportSnapshot();
        check("snapshot co xu", snap.coins == g.getCoins());
        check("snapshot co 2 o trong tui", snap.inventory.size() == 2);
        check("snapshot co 1 do dang gan", snap.equipment.size() == 1);

        Game h;
        check("nap snapshot OK", h.importSnapshot(snap).ok());
        check("sau nap game o NOT_STARTED", h.getStatus() == GameStatus::NOT_STARTED);
        check("xu giong nhau", h.getCoins() == g.getCoins());
        check("hunger giong nhau", h.getPet().getHunger() == g.getPet().getHunger());
        check("level giong nhau", h.getPet().getLevel() == g.getPet().getLevel());
        check("tui giong nhau (Tao vang)", h.getInventory().getQuantity(3) == g.getInventory().getQuantity(3));
        check("do dang gan giong nhau", h.getPet().getEquipped(DecorationSlot::HAT) == 7);

        Game bad; bad.start(); bad.buy(1);
        GameSnapshot sai = snap;
        sai.inventory.push_back({999, 1});
        check("snapshot co item la -> INVALID_SNAPSHOT", is(bad.importSnapshot(sai), ActionCode::INVALID_SNAPSHOT));
        check("nap loi thi van game cu khong doi", bad.getInventory().getQuantity(1) == 1 && bad.getStatus() == GameStatus::RUNNING);
        sai = snap; sai.pet.hunger = 150;
        check("chi so ngoai 0..100 -> INVALID_SNAPSHOT", is(bad.importSnapshot(sai), ActionCode::INVALID_SNAPSHOT));
        sai = snap; sai.coins = -1;
        check("xu am -> INVALID_SNAPSHOT", is(bad.importSnapshot(sai), ActionCode::INVALID_SNAPSHOT));
        sai = snap; sai.inventory.push_back({7, 1});
        check("trung item -> INVALID_SNAPSHOT", is(bad.importSnapshot(sai), ActionCode::INVALID_SNAPSHOT));
    }

    std::cout << "\nKet qua: " << passed << " dat, " << failed << " that bai\n";
    return failed == 0 ? 0 : 1;
}
