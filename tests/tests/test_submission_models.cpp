#include "Submission.h"
#include "TestCase.h"

#include <cassert>
#include <iostream>
#include <string>

void testSubmissionStatus() {
    Submission submission;

    assert(!submission.isFinished());
    assert(!submission.isAccepted());

    submission.status = SubmissionStatus::Accepted;
    assert(submission.isFinished());
    assert(submission.isAccepted());

    submission.status = SubmissionStatus::WrongAnswer;
    assert(submission.isFinished());
    assert(!submission.isAccepted());

    SubmissionStatus status;
    assert(submissionStatusFromString("queued", status));
    assert(status == SubmissionStatus::Queued);

    assert(submissionStatusFromString("accepted", status));
    assert(status == SubmissionStatus::Accepted);

    assert(submissionStatusFromString("compilation_error", status));
    assert(status == SubmissionStatus::CompilationError);

    assert(!submissionStatusFromString("unknown", status));
}

void testSubmissionValidation() {
    Submission submission;
    submission.userId = 1;
    submission.exerciseId = 1;
    submission.sourceCode = "int main() { return 0; }";
    submission.language = "cpp";

    std::string error;
    assert(validateSubmission(submission, error));
    assert(error.empty());

    submission.userId = 0;
    assert(!validateSubmission(submission, error));

    submission.userId = 1;
    submission.sourceCode.clear();
    assert(!validateSubmission(submission, error));

    submission.sourceCode = "int main() {}";
    submission.language = "python";
    assert(!validateSubmission(submission, error));

    submission.language = "cpp";
    submission.sourceCode = std::string(100, 'x');
    assert(!validateSubmission(submission, error, 50));
}

void testTestCaseValidation() {
    TestCase testCase;
    testCase.exerciseId = 1;
    testCase.input = "2 3";
    testCase.expectedOutput = "5";
    testCase.ordinal = 1;
    testCase.timeLimitMs = 2000;
    testCase.memoryLimitMb = 128;

    std::string error;
    assert(validateTestCase(testCase, error));
    assert(error.empty());

    testCase.exerciseId = 0;
    assert(!validateTestCase(testCase, error));

    testCase.exerciseId = 1;
    testCase.ordinal = 0;
    assert(!validateTestCase(testCase, error));

    testCase.ordinal = 1;
    testCase.expectedOutput.clear();
    assert(!validateTestCase(testCase, error));

    testCase.expectedOutput = "5";
    testCase.timeLimitMs = 50;
    assert(!validateTestCase(testCase, error));

    testCase.timeLimitMs = 2000;
    testCase.memoryLimitMb = 8;
    assert(!validateTestCase(testCase, error));
}

void testOutputComparison() {
    assert(outputsMatch("Hello\n", "Hello"));
    assert(outputsMatch("5   \n", "5"));
    assert(outputsMatch("1\n2\n", "1\n2"));

    assert(!outputsMatch("hello", "Hello"));
    assert(!outputsMatch("5", "6"));
    assert(!outputsMatch("1\n2", "1\n3"));

    assert(normalizeJudgeOutput("5   \r\n") == "5");
}

int main() {
    testSubmissionStatus();
    testSubmissionValidation();
    testTestCaseValidation();
    testOutputComparison();

    std::cout << "All Submission and TestCase tests passed.\n";
    return 0;
}