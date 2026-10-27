#include "Exercise.h"

#include <cassert>

int main() {
    assert(studyPointsPerCorrectAnswer("easy") == 15);
    assert(studyPointsPerCorrectAnswer("medium") == 18);
    assert(studyPointsPerCorrectAnswer("hard") == 20);
    assert(studyPointsPerCorrectAnswer("unknown") == 0);

    assert(studyCompletionBonus("easy") == 25);
    assert(studyCompletionBonus("medium") == 25);
    assert(studyCompletionBonus("hard") == 50);
    assert(studyCompletionCoins("hard") == 20);
    assert(studyCompletionCoins("easy") == 0);

    assert(studyStagePasses(4));
    assert(studyStagePasses(5));
    assert(!studyStagePasses(3));
    assert(!studyStagePasses(6));
    assert(!studyStagePasses(4, 4));
}
