#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>
#include <ctime>
#include <limits>
#include <cctype>
#include <string>

#include "Catalog.h"
#include "Crypto.h"
#include "Database.h"
#include "HttpServer.h"
#include "Json.h"
#include "Sha256.h"
#include "Submission.h"

#ifndef PET_FRONTEND_DIR
#define PET_FRONTEND_DIR "../../frontend"
#endif


namespace {
using json::Value;

// Submission throttling is process-local; submission records themselves are persisted in SQLite.
std::map<long long, std::deque<std::chrono::steady_clock::time_point>> submitTimes;
constexpr std::size_t MAX_SOURCE_BYTES = 32768;
constexpr std::size_t MAX_SUBMISSIONS_PER_MINUTE = 5;

std::string utcTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_MSC_VER)
    gmtime_s(&tm, &now);
#elif defined(_WIN32)
    if (const std::tm* utc = std::gmtime(&now)) tm = *utc;
#else
    gmtime_r(&now, &tm);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

Value submissionJson(const Submission& sub) {
    return {{"success",true},{"id",sub.id},{"submissionId",sub.id},
        {"userId",sub.userId},{"exerciseId",sub.exerciseId},
        {"language",sub.language},{"status",toString(sub.status)},
        {"score",sub.score},{"message",sub.message},
        {"submittedAt",sub.submittedAt},{"finishedAt",sub.finishedAt},
        {"finished",sub.isFinished()}};
}

HttpResponse response(int status, const Value& body) {
    HttpResponse result;
    result.status = status;
    result.body = body.dump();
    result.extraHeaders["Cache-Control"] = "no-store";
    result.extraHeaders["X-Content-Type-Options"] = "nosniff";
    return result;
}

HttpResponse error(int status, const std::string& message) {
    return response(status, {{"error", message}});
}

HttpResponse apiError(int status, const std::string& code, const std::string& message) {
    return response(status, {{"success",false},{"code",code},{"message",message}});
}

class AuthFailures {
    using Clock = std::chrono::steady_clock;
    std::deque<Clock::time_point> failures_;
public:
    int retryAfter() {
        auto now = Clock::now();
        while (!failures_.empty() && now - failures_.front() >= std::chrono::seconds(30))
            failures_.pop_front();
        if (failures_.size() < 10) return 0;
        return 30 - static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(now - failures_.front()).count());
    }
    void record() { failures_.push_back(Clock::now()); }
};

Value userJson(const UserRow& user) {
    return {{"id",user.id},{"username",user.username},{"email",user.email},
        {"coins",user.coins},{"soundEnabled",user.soundEnabled}};
}

Value petJson(const PetRow& pet) {
    return {{"name",pet.name},{"species",pet.species},{"speciesChosen",pet.speciesChosen},
        {"level",pet.level},{"experience",pet.experience},
        {"happiness",pet.happiness},{"hunger",pet.hunger},{"energy",pet.energy}};
}

Value progressJson(const ProgressRow& progress) {
    return {{"exerciseId",progress.exerciseId},{"status",progress.status},{"bestScore",progress.bestScore},
        {"attempts",progress.attempts},{"completedAt",progress.completedAt}};
}

Value studyStateJson(const StudyStateRow& state) {
    Value completed = Value::array();
    for (const auto& stage : state.completedStages) completed.push_back(stage);
    return {{"success",true},{"points",state.points},{"coins",state.coins},{"completedStages",completed},
        {"pet",{{"level",state.level},{"happiness",state.happiness},
            {"hunger",state.hunger},{"energy",state.energy}}}};
}

Value practiceStateJson(const PracticeStateRow& state) {
    return {{"success",true},{"points",state.points},{"bestScore",state.bestScore},{"coins",state.coins},
        {"pet",{{"level",state.level},{"happiness",state.happiness},
            {"hunger",state.hunger},{"energy",state.energy}}}};
}

Value inventoryJson(Database& db, long long userId) {
    Value result = Value::array();
    for (const auto& item : db.listInventory(userId)) result.push_back({{"itemId",item.id},{"quantity",item.quantity}});
    return result;
}

Value gameStateJson(Database& db, long long userId) {
    UserRow user; PetRow pet;
    if (!db.getUserById(userId,user) || !db.getPet(userId,pet))
        throw std::runtime_error("User game state not found");
    Value equipment = {{"headwear",nullptr},{"background",nullptr},{"floor",nullptr}};
    for (const auto& item : db.listEquipment(userId)) equipment[item.slot] = item.itemId;
    return {{"user",{{"id",user.id},{"username",user.username},{"coins",user.coins}}},
        {"pet",petJson(pet)},{"inventory",inventoryJson(db,userId)},
        {"equipment",equipment},{"settings",{{"soundEnabled",user.soundEnabled}}}};
}

HttpResponse actionResponse(Database& db, long long userId, const std::string& message, int levelsGained = 0) {
    const Value state = gameStateJson(db,userId);
    UserRow user;
    db.getUserById(userId,user);
    Value result = {{"success",true},{"code","OK"},{"message",message},
        {"levelsGained",levelsGained},{"state",state}};
    result["user"] = userJson(user);
    result["pet"] = state["pet"];
    result["inventory"] = state["inventory"];
    return response(200,result);
}

std::string stringField(const Value& fields, const char* name, size_t maximum) {
    if (!fields.count(name) || !fields[name].is_string()) throw std::invalid_argument("Expected string field");
    auto value = fields[name].get<std::string>();
    if (value.empty() || value.size() > maximum || value.find('\0') != std::string::npos)
        throw std::invalid_argument("Invalid string length or character");
    return value;
}

int integerField(const Value& fields, const char* name, int maximum) {
    if (!fields.count(name) || !fields[name].is_number_integer() || fields[name] < 1 || fields[name] > maximum)
        throw std::invalid_argument("Expected positive bounded integer");
    return fields[name].get<int>();
}

std::string tokenFor(const HttpRequest& request) {
    const auto auth = request.header("authorization");
    if (auth.compare(0,7,"Bearer ") == 0) return auth.substr(7);
    const auto cookies = request.header("cookie");
    size_t start = 0;
    while (start < cookies.size()) {
        const size_t end = cookies.find(';',start);
        const auto item = cookies.substr(start,end == std::string::npos ? std::string::npos : end-start);
        const size_t equals = item.find('=');
        if (equals != std::string::npos) {
            size_t keyStart = item.find_first_not_of(" \t");
            size_t keyEnd = item.find_last_not_of(" \t",equals-1);
            if (keyStart != std::string::npos && item.substr(keyStart,keyEnd-keyStart+1) == "vp_session")
                return item.substr(equals+1);
        }
        if (end == std::string::npos) break;
        start = end+1;
    }
    return "";
}

HttpResponse staticFile(const std::string& path) {
    if (path.empty() || path[0] != '/') return error(400,"Invalid path");
    if (path.find("..") != std::string::npos || path.find_first_of("%\\:\0",0,4) != std::string::npos)
        return error(403,"Forbidden path");
    std::string relative = path == "/" ? "/index.html" : path;
    auto dot = relative.rfind('.');
    std::string extension = dot == std::string::npos ? "" : relative.substr(dot);
    const std::map<std::string,std::string> types = {{".html","text/html; charset=utf-8"},
        {".css","text/css; charset=utf-8"},{".js","text/javascript; charset=utf-8"},
        {".png","image/png"},{".jpg","image/jpeg"},{".jpeg","image/jpeg"},
        {".gif","image/gif"},{".webp","image/webp"},{".svg","image/svg+xml"},
        {".ico","image/x-icon"},{".woff2","font/woff2"},{".mp3","audio/mpeg"}};
    auto type = types.find(extension);
    if (type == types.end()) return error(404,"File not found");
    std::ifstream input(std::string(PET_FRONTEND_DIR) + relative, std::ios::binary);
    if (!input) return error(404,"File not found");
    HttpResponse result;
    result.contentType = type->second;
    result.body.assign(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
    result.extraHeaders["Cache-Control"] = "no-store";
    result.extraHeaders["X-Content-Type-Options"] = "nosniff";
    return result;
}

HttpResponse route(Database& db, AuthFailures& authFailures, const HttpRequest& request) {
    if (request.method == "OPTIONS") {
        auto result = response(204,nullptr); result.body.clear(); return result;
    }
    if (request.method == "GET" && request.path.compare(0,5,"/api/") != 0) return staticFile(request.path);
    if (request.method == "GET" && request.path == "/api/shop") return response(200,shopCatalog());
    if (request.method == "GET" && request.path == "/api/exercises") {
        Value result = Value::array();
        for (const auto& exercise : db.listExercises()) result.push_back({{"id",exercise.id},
            {"title",exercise.title},{"description",exercise.description},
            {"difficulty",exercise.difficulty},{"maxScore",exercise.maxScore}});
        return response(200,result);
    }
    if (request.method == "POST" && request.path == "/api/auth/email-verification/request") {
        const auto fields = json::object(request.body);
        const std::string email = stringField(fields,"email",254);
        if (email.find('@') == std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid email address.");
        Value verificationCode = nullptr;
        const std::string candidate = crypto::randomHex(32);
        if (db.issueEmailVerification(email,sha256::hashHex(candidate),static_cast<long long>(std::time(nullptr))+1800))
            verificationCode = candidate;
        return response(200,{{"success",true},
            {"message","If the account needs verification, a code has been generated."},
            {"verificationCode",verificationCode},{"expiresInSeconds",1800}});
    }
    if (request.method == "POST" && request.path == "/api/auth/email-verification/confirm") {
        const auto fields = json::object(request.body);
        const std::string email = stringField(fields,"email",254);
        const std::string verificationCode = stringField(fields,"verificationCode",64);
        if (email.find('@') == std::string::npos || verificationCode.size() != 64 ||
            verificationCode.find_first_not_of("0123456789abcdef") != std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid email address and verification code.");
        if (!db.verifyEmail(email,sha256::hashHex(verificationCode)))
            return apiError(400,"INVALID_VERIFICATION_CODE","That verification code is invalid or expired. Request a new one.");
        return response(200,{{"success",true},{"message","Email verified. You can now log in."}});
    }
    if (request.method == "POST" && request.path == "/api/auth/password-reset/request") {
        const auto fields = json::object(request.body);
        const std::string email = stringField(fields,"email",254);
        if (email.find('@') == std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid email address.");

        Value resetCode = nullptr;
        const std::string candidate = crypto::randomHex(32);
        if (db.createPasswordReset(email,sha256::hashHex(candidate),static_cast<long long>(std::time(nullptr))+1800))
            resetCode = candidate;
        return response(200,{{"success",true},
            {"message","If the account exists, a reset code has been generated."},
            {"resetCode",resetCode},{"expiresInSeconds",1800}});
    }
    if (request.method == "POST" && request.path == "/api/auth/password-reset/confirm") {
        const auto fields = json::object(request.body);
        const std::string resetCode = stringField(fields,"resetCode",64);
        const std::string password = stringField(fields,"password",128);
        if (resetCode.size() != 64 || resetCode.find_first_not_of("0123456789abcdef") != std::string::npos ||
            password.size() < 8)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid reset code and a password of at least 8 characters.");
        const std::string salt = crypto::randomHex(16);
        if (!db.resetPassword(sha256::hashHex(resetCode),salt,crypto::passwordHash(password,salt)))
            return apiError(400,"INVALID_RESET_CODE","That reset code is invalid or expired. Request a new one.");
        return response(200,{{"success",true},{"message","Password reset. Please log in with your new password."}});
    }
    const bool registerPath = request.path == "/api/register" || request.path == "/api/auth/register";
    const bool loginPath = request.path == "/api/login" || request.path == "/api/auth/login";
    if (request.method == "POST" && (registerPath || loginPath)) {
        auto fields = json::object(request.body);
        auto email = stringField(fields,"email",254);
        auto password = stringField(fields,"password",1024);
        UserRow user;
        if (registerPath) {
            const bool modern = request.path == "/api/auth/register";
            std::string username;
            if (modern) username = stringField(fields,"username",20);
            const bool validUsername = !modern || (username.size() >= 3 &&
                username.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == std::string::npos);
            if (!validUsername || password.size() < (modern ? 8u : 6u) || email.find('@') == std::string::npos)
                return modern ? apiError(400,"INVALID_ARGUMENT","Enter a valid username, email, and password.")
                    : error(400,"Invalid email or password");
            if (db.emailExists(email)) return modern ? apiError(409,"EMAIL_TAKEN","This email is already registered.")
                : error(409,"Email already registered");
            if (modern && db.usernameExists(username)) return apiError(409,"USERNAME_TAKEN","This username is already taken.");
            db.createUser(username,email,password,user);
            if (modern) return response(201,{{"success",true},{"code","REGISTERED"},
                {"message","Account created successfully."},
                {"user",{{"id",user.id},{"username",user.username}}}});
            return response(201,{{"success",true},{"message","Account created successfully."}});
        } else {
            int retry = authFailures.retryAfter();
            if (retry) {
                auto result = error(429,"Too many failed logins; try again later");
                result.extraHeaders["Retry-After"] = std::to_string(retry);
                return request.path == "/api/auth/login"
                    ? apiError(429,"RATE_LIMITED","Too many failed logins; try again later.") : result;
            }
            if (!db.verifyLogin(email,password,user)) {
                authFailures.record();
                return request.path == "/api/auth/login"
                    ? apiError(401,"INVALID_CREDENTIALS","Invalid email or password.") : error(401,"Invalid credentials");
            }
            db.touchLastLogin(user.id);
        }
        const std::string token = db.createSession(user.id);
        if (request.path == "/api/auth/login") {
            auto result = response(200,{{"success",true},{"code","AUTHENTICATED"},
                {"user",{{"id",user.id},{"username",user.username}}}});
            result.extraHeaders["Set-Cookie"] = "vp_session="+token+"; Path=/; HttpOnly; SameSite=Strict; Max-Age=604800";
            return result;
        }
        return response(request.path == "/api/register" ? 201 : 200,
            {{"token",token},{"user",userJson(user)}});
    }
    if (request.method == "GET" && request.path == "/api/auth/session") {
        long long sessionUserId = 0;
        UserRow user;
        const bool authenticated = db.userIdForToken(tokenFor(request),sessionUserId) && db.getUserById(sessionUserId,user);
        return response(200,{{"success",true},{"authenticated",authenticated},
            {"user",authenticated ? Value{{"id",user.id},{"username",user.username}} : Value(nullptr)}});
    }
    if (request.method == "POST" && request.path == "/api/auth/logout") {
        const std::string token = tokenFor(request);
        if (!token.empty()) db.revokeSession(token);
        auto result = response(200,{{"success",true},{"code","LOGGED_OUT"},{"message","Signed out."}});
        result.extraHeaders["Set-Cookie"] = "vp_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0";
        return result;
    }
    long long userId = 0;
    const auto token = tokenFor(request);
    if (!db.userIdForToken(token,userId)) return error(401,"Authentication required");
    if (request.method == "GET" && request.path == "/api/account/profile") {
        UserRow user;
        if (!db.getUserById(userId,user)) return apiError(404,"USER_NOT_FOUND","Account not found.");
        return response(200,{{"user",{{"username",user.username},{"email",user.email},
            {"emailVerified",user.emailVerified}}}});
    }
    if (request.method == "PUT" && request.path == "/api/account/password") {
        const auto fields = json::object(request.body);
        const std::string currentPassword = stringField(fields,"currentPassword",128);
        const std::string newPassword = stringField(fields,"newPassword",128);
        if (fields.size() != 2 || newPassword.size() < 8)
            return apiError(400,"INVALID_ARGUMENT","The new password must be at least 8 characters.");
        if (!db.changePassword(userId,currentPassword,newPassword,sha256::hashHex(token)))
            return apiError(403,"CURRENT_PASSWORD_INVALID","The current password is incorrect.");
        return response(200,{{"success",true},{"message","Password changed. Other sessions have been signed out."}});
    }
    if (request.method == "POST" && request.path == "/api/account/email/request") {
        const auto fields = json::object(request.body);
        const std::string currentPassword = stringField(fields,"currentPassword",128);
        const std::string newEmail = stringField(fields,"newEmail",254);
        if (fields.size() != 2 || newEmail.find('@') == std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid new email address.");
        UserRow account, verifiedAccount;
        if (!db.getUserById(userId,account) ||
            !db.verifyLogin(account.email,currentPassword,verifiedAccount) || verifiedAccount.id != userId)
            return apiError(403,"CURRENT_PASSWORD_INVALID","The current password is incorrect.");
        if (db.emailExists(newEmail)) return apiError(409,"EMAIL_TAKEN","That email is already registered.");
        const std::string verificationCode = crypto::randomHex(32);
        if (!db.createPendingEmailChange(userId,newEmail,sha256::hashHex(verificationCode),
                                        static_cast<long long>(std::time(nullptr))+1800))
            return apiError(409,"EMAIL_TAKEN","That email is already registered or awaiting verification.");
        return response(200,{{"success",true},
            {"message","Verify the new email to finish changing your account email."},
            {"verificationCode",verificationCode},{"expiresInSeconds",1800}});
    }
    if (request.method == "POST" && request.path == "/api/account/email/confirm") {
        const auto fields = json::object(request.body);
        const std::string email = stringField(fields,"email",254);
        const std::string verificationCode = stringField(fields,"verificationCode",64);
        if (fields.size() != 2 || email.find('@') == std::string::npos || verificationCode.size() != 64 ||
            verificationCode.find_first_not_of("0123456789abcdef") != std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Enter a valid email address and verification code.");
        if (!db.confirmEmailChange(userId,email,sha256::hashHex(verificationCode)))
            return apiError(400,"INVALID_VERIFICATION_CODE","That code is invalid or expired. Request a new one.");
        return response(200,{{"success",true},{"message","Email address updated and verified."}});
    }
    if (request.method == "POST" && request.path == "/api/logout") {
        db.revokeSession(token); return response(200,{{"ok",true}});
    }
    if (request.method == "GET" && request.path == "/api/game/state")
        return response(200,gameStateJson(db,userId));
    if (request.method == "GET" && request.path == "/api/study/state") {
        StudyStateRow state;
        if (!db.getStudyState(userId,state)) return apiError(404,"STUDY_STATE_NOT_FOUND","Study state not found.");
        return response(200,studyStateJson(state));
    }
    if (request.method == "POST" && request.path == "/api/study/time") {
        const auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("action") || !fields["action"].is_string())
            return apiError(400,"INVALID_ARGUMENT","Expected a Study time action.");
        StudyStateRow state;
        const auto status = db.updateStudyActivity(userId,fields["action"].get<std::string>(),
                                                    static_cast<long long>(std::time(nullptr)),state);
        if (status == StudyOperationStatus::InvalidAction)
            return apiError(400,"INVALID_ARGUMENT","Unknown Study time action.");
        if (status == StudyOperationStatus::UserNotFound)
            return apiError(404,"STUDY_STATE_NOT_FOUND","Study state not found.");
        Value result = studyStateJson(state);
        result["blocked"] = status == StudyOperationStatus::PetNeedsCare;
        return response(200,result);
    }
    if (request.method == "POST" && request.path == "/api/study/complete") {
        const auto fields = json::object(request.body);
        if (fields.size() != 3 || !fields.count("level") || !fields["level"].is_string() ||
            !fields.count("tier") || !fields["tier"].is_string() ||
            !fields.count("correctAnswers") || !fields["correctAnswers"].is_number_integer() ||
            fields["correctAnswers"] < 0 || fields["correctAnswers"] > 5)
            return apiError(400,"INVALID_ARGUMENT","Expected level, tier, and 0–5 correct answers.");

        StudyStateRow state;
        bool passed = false, review = false;
        int awardedPoints = 0, awardedCoins = 0;
        const auto status = db.completeStudyStage(userId,fields["level"].get<std::string>(),
            fields["tier"].get<std::string>(),fields["correctAnswers"].get<int>(),
            state,passed,review,awardedPoints,awardedCoins);
        if (status == StudyOperationStatus::PetNeedsCare)
            return apiError(409,"PET_NEEDS_CARE","Your pet needs care before continuing Study.");
        if (status == StudyOperationStatus::StageLocked)
            return apiError(409,"STAGE_LOCKED","Complete the previous Study stage first.");
        if (status == StudyOperationStatus::InvalidStage || status == StudyOperationStatus::InvalidAnswerCount)
            return apiError(400,"INVALID_ARGUMENT","Invalid Study stage or answer count.");
        if (status == StudyOperationStatus::UserNotFound)
            return apiError(404,"STUDY_STATE_NOT_FOUND","Study state not found.");
        Value result = {{"success",true},{"passed",passed},{"review",review},
            {"correctAnswers",fields["correctAnswers"]},{"awardedPoints",awardedPoints},
            {"awardedCoins",awardedCoins},{"state",studyStateJson(state)}};
        return response(200,result);
    }
    if (request.method == "GET" && request.path == "/api/practice/state") {
        PracticeStateRow state;
        if (!db.getPracticeState(userId,state))
            return apiError(404,"PRACTICE_STATE_NOT_FOUND","Practice state not found.");
        return response(200,practiceStateJson(state));
    }
    if (request.method == "POST" && request.path == "/api/practice/time") {
        const auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("action") || !fields["action"].is_string())
            return apiError(400,"INVALID_ARGUMENT","Expected a Practice time action.");
        PracticeStateRow state;
        const auto status = db.updatePracticeActivity(userId,fields["action"].get<std::string>(),
                                                       static_cast<long long>(std::time(nullptr)),state);
        if (status == PracticeOperationStatus::InvalidAction)
            return apiError(400,"INVALID_ARGUMENT","Unknown Practice time action.");
        if (status == PracticeOperationStatus::UserNotFound)
            return apiError(404,"PRACTICE_STATE_NOT_FOUND","Practice state not found.");
        return response(200,practiceStateJson(state));
    }
    if (request.method == "POST" && request.path == "/api/practice/game/start") {
        const auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("gameId") || !fields["gameId"].is_string())
            return apiError(400,"INVALID_ARGUMENT","Expected a Practice game ID.");
        const std::string gameId = fields["gameId"].get<std::string>();
        if (gameId.size() != 36 || gameId.find_first_not_of("0123456789abcdef-") != std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Invalid Practice game ID.");
        if (!db.startPracticeGame(userId,gameId))
            return apiError(404,"PRACTICE_STATE_NOT_FOUND","Practice state not found.");
        return response(200,{{"success",true}});
    }
    if (request.method == "POST" && request.path == "/api/practice/game/abandon") {
        const auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("gameId") || !fields["gameId"].is_string())
            return apiError(400,"INVALID_ARGUMENT","Expected a Practice game ID.");
        const std::string gameId = fields["gameId"].get<std::string>();
        if (gameId.size() != 36 || gameId.find_first_not_of("0123456789abcdef-") != std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Invalid Practice game ID.");
        if (!db.abandonPracticeGame(userId,gameId))
            return apiError(404,"PRACTICE_STATE_NOT_FOUND","Practice state not found.");
        return response(200,{{"success",true}});
    }
    if (request.method == "POST" && request.path == "/api/practice/finish") {
        const auto fields = json::object(request.body);
        if (fields.size() != 2 || !fields.count("gameId") || !fields["gameId"].is_string() ||
            !fields.count("eaten") || !fields["eaten"].is_number_integer() ||
            fields["eaten"] < 0 || fields["eaten"] > 1000000)
            return apiError(400,"INVALID_ARGUMENT","Expected a Practice game ID and eaten count.");
        const std::string gameId = fields["gameId"].get<std::string>();
        if (gameId.size() != 36 || gameId.find_first_not_of("0123456789abcdef-") != std::string::npos)
            return apiError(400,"INVALID_ARGUMENT","Invalid Practice game ID.");
        PracticeStateRow state;
        int awardedPoints = 0, awardedCoins = 0;
        bool duplicate = false;
        const auto status = db.finishPracticeGame(userId,gameId,fields["eaten"].get<int>(),
                                                   state,awardedPoints,awardedCoins,duplicate);
        if (status == PracticeOperationStatus::GameNotActive)
            return apiError(409,"PRACTICE_GAME_NOT_ACTIVE","This Practice game is no longer active.");
        if (status == PracticeOperationStatus::UserNotFound)
            return apiError(404,"PRACTICE_STATE_NOT_FOUND","Practice state not found.");
        return response(200,{{"success",true},{"awardedPoints",awardedPoints},
            {"awardedCoins",awardedCoins},{"duplicate",duplicate},{"state",practiceStateJson(state)}});
    }
    if (request.method == "GET" && request.path == "/api/me") {
        UserRow user; PetRow pet;
        if (!db.getUserById(userId,user) || !db.getPet(userId,pet)) return error(404,"User not found");
        Value progress = Value::array();
        for (const auto& row : db.listProgress(userId)) progress.push_back(progressJson(row));
        return response(200,{{"user",userJson(user)},{"pet",petJson(pet)},
            {"progress",progress},{"inventory",inventoryJson(db,userId)}});
    }
    if (request.method == "POST" && request.path == "/api/shop/purchases") {
        auto fields = json::object(request.body);
        int itemId = integerField(fields,"itemId",2147483647);
        int quantity = integerField(fields,"quantity",999);
        int status = db.purchase(userId,itemId,quantity);
        if (status != 200) return apiError(status,"PURCHASE_FAILED","Unknown item, insufficient coins, or inventory limit.");
        return actionResponse(db,userId,"Purchase complete.");
    }
    if (request.method == "POST" && request.path == "/api/pet/feed") {
        auto fields = json::object(request.body);
        int itemId = integerField(fields,"itemId",2147483647);
        int status = db.feed(userId,itemId);
        if (status != 200) return apiError(status,status == 400 ? "NOT_FOOD" : status == 409 ? "NOT_OWNED" : "ITEM_NOT_FOUND",
            status == 400 ? "Item is not food." : status == 409 ? "Food not owned." : "Item or pet not found.");
        return actionResponse(db,userId,"Food eaten.");
    }
    if (request.method == "PUT" && request.path == "/api/pet") {
        auto fields = json::object(request.body);
        if (!fields.count("name") || fields.size() > 2 || (fields.size() == 2 && !fields.count("species")))
            return apiError(400,"INVALID_ARGUMENT","Only pet name and species may be updated.");
        auto name = stringField(fields,"name",80);
        PetRow pet;
        if (!db.getPet(userId,pet)) return apiError(404,"PET_NOT_FOUND","Pet not found.");
        const std::string species = fields.count("species") ? stringField(fields,"species",8) : pet.species;
        if (species != "cat" && species != "dog") return apiError(400,"INVALID_ARGUMENT","Species must be cat or dog.");
        if (!db.savePetProfile(userId,name,species)) return apiError(404,"PET_NOT_FOUND","Pet not found.");
        return actionResponse(db,userId,"Pet profile saved.");
    }
    if (request.method == "PUT" && request.path == "/api/settings") {
        auto fields = json::object(request.body);
        if (!fields.count("soundEnabled") || !fields["soundEnabled"].is_boolean() || fields.size() != 1)
            return apiError(400,"INVALID_ARGUMENT","soundEnabled must be true or false.");
        if (!db.setSoundEnabled(userId,fields["soundEnabled"].get<bool>()))
            return apiError(404,"USER_NOT_FOUND","User not found.");
        return actionResponse(db,userId,"Settings saved.");
    }
    if (request.method == "POST" && (request.path == "/api/pet/play" ||
        request.path == "/api/pet/sleep" || request.path == "/api/pet/bath")) {
        if (!request.body.empty()) return apiError(400,"INVALID_ARGUMENT","This action does not accept a body.");
        const std::string action = request.path.substr(std::string("/api/pet/").size());
        int levelsGained = 0;
        const int status = db.performPetAction(userId,action,levelsGained);
        if (status != 200) return apiError(status,status == 409 ? (action == "sleep" ? "ENERGY_FULL" : "PET_TOO_TIRED") : "PET_NOT_FOUND",
            status == 409 ? (action == "sleep" ? "Your pet is not sleepy." : "Your pet is too tired to play.") : "Pet not found.");
        const std::string message = action == "sleep" ? "Your pet slept well." : action == "play" ? "Your pet had fun!" : "Squeaky clean!";
        return actionResponse(db,userId,message,levelsGained);
    }
    if (request.method == "POST" && request.path == "/api/pet/decorate") {
        auto fields = json::object(request.body);
        const int itemId = integerField(fields,"itemId",2147483647);
        const int status = db.equipDecoration(userId,itemId);
        if (status != 200) return apiError(status,status == 400 ? "NOT_DECORATION" : status == 409 ? "NOT_OWNED" : "ITEM_NOT_FOUND",
            status == 400 ? "Item is not a decoration." : status == 409 ? "You do not own this decoration." : "Item not found.");
        return actionResponse(db,userId,"Decoration equipped.");
    }
    if (request.method == "POST" && request.path == "/api/pet/unequip") {
        auto fields = json::object(request.body);
        const std::string slot = stringField(fields,"slot",16);
        if (fields.size() != 1 || !db.unequipDecoration(userId,slot))
            return apiError(400,"INVALID_ARGUMENT","slot must be headwear, background, or floor.");
        return actionResponse(db,userId,"Decoration removed.");
    }
    const std::string prefix = "/api/exercises/", suffix = "/submissions";
    const auto& path = request.path;

    // SUB-02: submit source code asynchronously; existing {answer: ...} quiz API remains unchanged.
    if (request.method == "POST" && path.compare(0,prefix.size(),prefix) == 0 &&
        path.size() > prefix.size()+suffix.size() && path.compare(path.size()-suffix.size(),suffix.size(),suffix) == 0) {
        const std::string exerciseText = path.substr(prefix.size(),path.size()-prefix.size()-suffix.size());
        if (exerciseText.empty() || exerciseText.find_first_not_of("0123456789") != std::string::npos)
            return apiError(404,"EXERCISE_NOT_FOUND","Exercise not found.");
        long long exerciseId = 0;
        try { exerciseId = std::stoll(exerciseText); } catch (...) { return apiError(404,"EXERCISE_NOT_FOUND","Exercise not found."); }
        bool exists = false;
        for (const auto& exercise : db.listExercises()) if (exercise.id == exerciseId) { exists = true; break; }
        if (!exists) return apiError(404,"EXERCISE_NOT_FOUND","Exercise not found.");

        auto fields = json::object(request.body);
        if (fields.count("sourceCode")) {
            if (!fields["sourceCode"].is_string())
                return apiError(400,"INVALID_SOURCE","sourceCode must be a string.");
            Submission sub;
            sub.userId = userId;
            sub.exerciseId = exerciseId;
            sub.sourceCode = fields["sourceCode"].get<std::string>();
            sub.language = fields.count("language") && fields["language"].is_string()
                ? fields["language"].get<std::string>() : "cpp";
            std::string validationError;
            if (!validateSubmission(sub, validationError, MAX_SOURCE_BYTES))
                return apiError(400,"INVALID_SUBMISSION",validationError);

            auto& times = submitTimes[userId];
            const auto now = std::chrono::steady_clock::now();
            while (!times.empty() && now - times.front() >= std::chrono::minutes(1)) times.pop_front();
            if (times.size() >= MAX_SUBMISSIONS_PER_MINUTE) {
                auto result = apiError(429,"RATE_LIMITED","Maximum 5 code submissions per minute.");
                result.extraHeaders["Retry-After"] = "60";
                return result;
            }
            times.push_back(now);
            sub.status = SubmissionStatus::Queued;
            sub.score = 0;
            sub.message = "Submission queued. The evaluator worker will process it asynchronously.";
            sub.submittedAt = utcTimestamp();
            if (!db.createCodeSubmission(sub))
                return apiError(500,"SUBMISSION_STORAGE_ERROR","Could not persist submission.");
            auto result = response(202,submissionJson(sub));
            result.extraHeaders["Location"] = "/api/submissions/" + std::to_string(sub.id);
            return result;
        }
    }

    // SUB-02: poll status. Users can only read their own submissions.
    const std::string submissionPrefix = "/api/submissions/";
    if (request.method == "GET" && path.compare(0,submissionPrefix.size(),submissionPrefix) == 0) {
        const std::string idText = path.substr(submissionPrefix.size());
        if (idText.empty() || idText.find_first_not_of("0123456789") != std::string::npos)
            return apiError(400,"INVALID_SUBMISSION_ID","Invalid submission ID.");
        long long id = 0;
        try { id = std::stoll(idText); } catch (...) { return apiError(400,"INVALID_SUBMISSION_ID","Invalid submission ID."); }
        Submission sub;
        if (!db.getCodeSubmission(id, sub)) return apiError(404,"SUBMISSION_NOT_FOUND","Submission not found.");
        if (sub.userId != userId) return apiError(403,"FORBIDDEN","You cannot view this submission.");
        return response(200,submissionJson(sub));
    }

    if (request.method == "POST" && path.compare(0,prefix.size(),prefix) == 0 &&
        path.size() > prefix.size()+suffix.size() && path.compare(path.size()-suffix.size(),suffix.size(),suffix) == 0) {
        auto id = path.substr(prefix.size(),path.size()-prefix.size()-suffix.size());
        if (id.size() != 1 || id[0] < '1' || id[0] > '5') return error(404,"Exercise not found");
        auto fields = json::object(request.body);
        if (!fields.count("answer") || !fields["answer"].is_string() || fields["answer"].get_ref<const std::string&>().size() > 8192)
            return error(400,"Expected answer string (maximum 8192 bytes)");
        int score = 0; ProgressRow progress;
        if (!db.recordSubmission(userId,id[0]-'0',fields["answer"].get<std::string>(),score,progress))
            return error(404,"Exercise not found");
        return response(201,{{"score",score},{"status",score == 100 ? "completed" : "in_progress"},
            {"progress",progressJson(progress)}});
    }
    return error(404,"Endpoint not found");
}
}

int main(int argc, char** argv) {
    try {
        int port = argc >= 2 ? std::stoi(argv[1]) : 8080;
        if (port < 1 || port > 65535) throw std::invalid_argument("Invalid port");
        Database db;
        if (!db.open(argc >= 3 ? argv[2] : "backend/build/virtual_pet.db") || !db.runSchema() || !db.seedExercisesIfEmpty())
            throw std::runtime_error(db.lastError());
        AuthFailures authFailures;
        HttpServer server;
        server.setHandler([&db,&authFailures,port](const HttpRequest& request) {
            const auto origin = request.header("origin");
            const auto host = request.header("host");
            if (host != "127.0.0.1:"+std::to_string(port) && host != "localhost:"+std::to_string(port))
                return error(403,"Local host required");
            bool localOrigin = origin == "http://localhost:5500" || origin == "http://127.0.0.1:5500" || origin == "null";
            bool sameOrigin = origin == "http://"+host;
            if (!origin.empty() && !localOrigin && !sameOrigin) return error(403,"Origin not allowed");
            HttpResponse result;
            try { result = route(db,authFailures,request); }
            catch (const nlohmann::json::exception&) { result = error(400,"Invalid JSON or field type"); }
            catch (const std::invalid_argument& exception) { result = error(400,exception.what()); }
            catch (const std::exception& exception) {
                std::fprintf(stderr,"Request failed: %s\n",exception.what());
                result = error(500,"Internal server error");
            }
            if (localOrigin || sameOrigin) {
                result.extraHeaders["Access-Control-Allow-Origin"] = origin;
                result.extraHeaders["Vary"] = "Origin";
                result.extraHeaders["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
                result.extraHeaders["Access-Control-Allow-Methods"] = "GET, POST, PUT, OPTIONS";
            }
            return result;
        });
        if (!server.listen("127.0.0.1",port)) throw std::runtime_error(server.lastError());
    } catch (const std::exception& exception) {
        std::fprintf(stderr,"Server failed: %s\n",exception.what()); return 1;
    }
    return 0;
}
