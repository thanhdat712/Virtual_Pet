#include "Exercise.h"

int studyPointsPerCorrectAnswer(const std::string &tier) noexcept
{
    if (tier == "easy")
        return 15;
    if (tier == "medium")
        return 18;
    if (tier == "hard")
        return 20;
    return 0;
}

int studyCompletionBonus(const std::string &tier) noexcept
{
    return tier == "hard" ? 50 : (tier == "easy" || tier == "medium" ? 25 : 0);
}

int studyCompletionCoins(const std::string &tier) noexcept
{
    return tier == "hard" ? 20 : 0;
}

bool studyStagePasses(int correctAnswers, int questionCount) noexcept
{
    return questionCount == 5 && correctAnswers >= 4 && correctAnswers <= questionCount;
}