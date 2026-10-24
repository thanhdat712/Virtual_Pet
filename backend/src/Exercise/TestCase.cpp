#include "TestCase.h"

#include <algorithm>
#include <cctype>

namespace {
std::string trimRight(std::string value) {
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'))
        value.pop_back();
    return value;
}
}

std::string normalizeJudgeOutput(const std::string& output) {
    std::string normalized;
    std::string line;
    for (char ch : output) {
        if (ch == '\n') {
            normalized += trimRight(line);
            normalized.push_back('\n');
            line.clear();
        } else {
            line.push_back(ch);
        }
    }
    if (!line.empty()) normalized += trimRight(line);
    while (!normalized.empty() && normalized.back() == '\n') normalized.pop_back();
    return normalized;
}

bool outputsMatch(const std::string& actual, const std::string& expected) {
    return normalizeJudgeOutput(actual) == normalizeJudgeOutput(expected);
}

bool validateTestCase(const TestCase& testCase, std::string& error) noexcept {
    if (testCase.exerciseId <= 0) { error = "exerciseId must be positive"; return false; }
    if (testCase.ordinal < 1) { error = "ordinal must be at least 1"; return false; }
    if (testCase.expectedOutput.empty()) { error = "expectedOutput must not be empty"; return false; }
    if (testCase.timeLimitMs < 100 || testCase.timeLimitMs > 10000) {
        error = "timeLimitMs must be between 100 and 10000"; return false;
    }
    if (testCase.memoryLimitMb < 16 || testCase.memoryLimitMb > 512) {
        error = "memoryLimitMb must be between 16 and 512"; return false;
    }
    error.clear();
    return true;
}
