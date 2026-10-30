#ifndef VIRTUAL_PET_EXERCISE_EXERCISE_H
#define VIRTUAL_PET_EXERCISE_EXERCISE_H

#include <string>

int studyPointsPerCorrectAnswer(const std::string &tier) noexcept;
int studyCompletionBonus(const std::string &tier) noexcept;
int studyCompletionCoins(const std::string &tier) noexcept;
bool studyStagePasses(int correctAnswers, int questionCount = 5) noexcept;

#endif