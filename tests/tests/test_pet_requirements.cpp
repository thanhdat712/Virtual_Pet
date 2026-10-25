
#include <iostream>
#include "Pet.h"
#include "Cat.h"
#include "Dog.h"
#include "Food.h"

namespace {
int passed = 0;
int failed = 0;

void check(const char* label, bool condition) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << label << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << label << '\n';
    }
}
}

int main() {
    // PET-06: stat clamping and valid starting level.
    Cat clamped(1, "meomeo", 0, 150, -5, 120, -1);
    check("level is at least 1", clamped.getLevel() == 1);
    check("health clamps to 100", clamped.getHealth() == 100);
    check("happiness clamps to 0", clamped.getHappiness() == 0);
    check("hunger clamps to 100", clamped.getHunger() == 100);
    check("energy clamps to 0", clamped.getEnergy() == 0);

    // Feeding increases fullness (hunger stat represents fullness: 100 = full).
    Cat fed(2, "meomeo", 1, 100, 10, 80, 50);
    Food food(10, "Snack", "Test food", 1, "", 30, 5);
    auto feedResult = fed.feed(food);
    check("feeding succeeds for a healthy pet", feedResult.ok());
    check("feeding clamps fullness at 100", fed.getHunger() == 100);
    check("feeding restores happiness", fed.getHappiness() == 15);

    // Playing consumes energy and fullness and increases happiness.
    Cat playful(3, "meomeo", 1, 100, 20, 50, 50);
    auto playResult = playful.play();
    check("play succeeds with enough energy", playResult.ok());
    check("play consumes energy", playful.getEnergy() == 35);
    check("play consumes fullness", playful.getHunger() == 40);
    check("play increases happiness", playful.getHappiness() == 35);

    Cat tired(4, "meomeo", 1, 100, 20, 50, 5);
    check("play fails when energy is insufficient", !tired.play().ok());
    check("failed play does not consume energy", tired.getEnergy() == 5);

    Cat sleeping(5, "meomeo", 1, 100, 20, 50, 20);
    auto sleepResult = sleeping.sleep();
    check("sleep succeeds when energy is not full", sleepResult.ok());
    check("sleep restores energy with upper clamp", sleeping.getEnergy() == 60);
    check("sleep consumes fullness", sleeping.getHunger() == 45);

    Cat bathing(6, "meomeo", 1, 100, 20, 50, 50);
    auto bathResult = bathing.bath();
    check("bath succeeds for a healthy pet", bathResult.ok());
    check("bath costs energy", bathing.getEnergy() == 45);
    check("bath increases happiness", bathing.getHappiness() == 25);

    Cat elapsed(7, "meomeo", 1, 100, 50, 50, 50);
    elapsed.applyTimePassed(10);
    check("time passage decreases fullness", elapsed.getHunger() == 40);
    check("time passage decreases energy", elapsed.getEnergy() == 40);
    elapsed.applyTimePassed(0);
    check("non-positive elapsed ticks do nothing",
          elapsed.getHunger() == 40 && elapsed.getEnergy() == 40);

    Dog dog(8, "gaugau", 1, 100, 20, 50, 50);
    Pet* pet = &dog;
    check("polymorphic type reports dog", pet->getType() == "Chó");
    check("polymorphic play works", pet->play().ok());

    std::cout << "\nPET-06 tests: " << passed
              << " passed, " << failed << " failed.\n";

    return failed == 0 ? 0 : 1;
}
