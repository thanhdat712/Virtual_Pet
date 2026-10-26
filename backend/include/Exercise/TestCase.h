#ifndef VIRTUAL_PET_EXERCISE_TEST_CASE_H
#define VIRTUAL_PET_EXERCISE_TEST_CASE_H

#include <string>

struct TestCase {
    long long id = 0;
    long long exerciseId = 0;
    std::string input;
    std::string expectedOutput;
    bool hidden = false;
    int ordinal = 1;
    int timeLimitMs = 2000;
    int memoryLimitMb = 128;
};

// Judge comparison ignores trailing spaces on each line and final blank lines,
// but preserves meaningful internal whitespace and case.
std::string normalizeJudgeOutput(const std::string& output);
bool outputsMatch(const std::string& actual, const std::string& expected);
bool validateTestCase(const TestCase& testCase, std::string& error) noexcept;

#endif
