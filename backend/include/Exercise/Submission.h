#ifndef VIRTUAL_PET_EXERCISE_SUBMISSION_H
#define VIRTUAL_PET_EXERCISE_SUBMISSION_H

#include <ctime>
#include <string>

// Values intentionally map to the submission lifecycle used by the HTTP API.
enum class SubmissionStatus {
    Queued,
    Evaluating,
    Accepted,
    WrongAnswer,
    CompilationError,
    RuntimeError,
    TimeLimitExceeded,
    MemoryLimitExceeded,
    InternalError
};

struct Submission {
    long long id = 0;
    long long userId = 0;
    long long exerciseId = 0;
    std::string sourceCode;
    std::string language = "cpp";
    SubmissionStatus status = SubmissionStatus::Queued;
    int score = 0;
    std::string message;
    std::string submittedAt; // ISO-8601 timestamp as stored by SQLite
    std::string finishedAt;  // Empty until evaluation finishes

    bool isFinished() const noexcept;
    bool isAccepted() const noexcept;
};

const char* toString(SubmissionStatus status) noexcept;
bool submissionStatusFromString(const std::string& value, SubmissionStatus& out) noexcept;
bool validateSubmission(const Submission& submission, std::string& error,
                        std::size_t maxSourceBytes = 32768) noexcept;

#endif
