#include "Submission.h"

bool Submission::isFinished() const noexcept {
    return status != SubmissionStatus::Queued && status != SubmissionStatus::Evaluating;
}

bool Submission::isAccepted() const noexcept { return status == SubmissionStatus::Accepted; }

const char* toString(SubmissionStatus status) noexcept {
    switch (status) {
        case SubmissionStatus::Queued: return "queued";
        case SubmissionStatus::Evaluating: return "evaluating";
        case SubmissionStatus::Accepted: return "accepted";
        case SubmissionStatus::WrongAnswer: return "wrong_answer";
        case SubmissionStatus::CompilationError: return "compile_error";
        case SubmissionStatus::RuntimeError: return "runtime_error";
        case SubmissionStatus::TimeLimitExceeded: return "time_limit_exceeded";
        case SubmissionStatus::MemoryLimitExceeded: return "memory_limit_exceeded";
        case SubmissionStatus::InternalError: return "internal_error";
    }
    return "internal_error";
}

bool submissionStatusFromString(const std::string& value, SubmissionStatus& out) noexcept {
    if (value == "queued" || value == "submitted") out = SubmissionStatus::Queued;
    else if (value == "evaluating") out = SubmissionStatus::Evaluating;
    else if (value == "accepted") out = SubmissionStatus::Accepted;
    else if (value == "wrong_answer") out = SubmissionStatus::WrongAnswer;
    else if (value == "compile_error" || value == "compilation_error") out = SubmissionStatus::CompilationError;
    else if (value == "runtime_error") out = SubmissionStatus::RuntimeError;
    else if (value == "time_limit_exceeded") out = SubmissionStatus::TimeLimitExceeded;
    else if (value == "memory_limit_exceeded") out = SubmissionStatus::MemoryLimitExceeded;
    else if (value == "internal_error") out = SubmissionStatus::InternalError;
    else return false;
    return true;
}

bool validateSubmission(const Submission& submission, std::string& error,
                        std::size_t maxSourceBytes) noexcept {
    if (submission.userId <= 0) { error = "userId must be positive"; return false; }
    if (submission.exerciseId <= 0) { error = "exerciseId must be positive"; return false; }
    if (submission.sourceCode.empty()) { error = "sourceCode must not be empty"; return false; }
    if (submission.sourceCode.size() > maxSourceBytes) { error = "sourceCode exceeds size limit"; return false; }
    if (submission.sourceCode.find('\0') != std::string::npos) { error = "sourceCode contains NUL byte"; return false; }
    if (submission.language != "cpp") { error = "unsupported language"; return false; }
    if (submission.score < 0) { error = "score must not be negative"; return false; }
    error.clear();
    return true;
}
