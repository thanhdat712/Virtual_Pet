#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iterator>
#include <string>

#include "Catalog.h"
#include "Database.h"
#include "HttpServer.h"
#include "Json.h"

namespace {
using json::Value;

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
    return {{"id",user.id},{"email",user.email},{"coins",user.coins}};
}

Value petJson(const PetRow& pet) {
    return {{"name",pet.name},{"species",pet.species},{"level",pet.level},{"experience",pet.experience},
        {"happiness",pet.happiness},{"hunger",pet.hunger},{"energy",pet.energy}};
}

Value progressJson(const ProgressRow& progress) {
    return {{"exerciseId",progress.exerciseId},{"status",progress.status},{"bestScore",progress.bestScore},
        {"attempts",progress.attempts},{"completedAt",progress.completedAt}};
}

Value inventoryJson(Database& db, long long userId) {
    Value result = Value::array();
    for (const auto& item : db.listInventory(userId)) result.push_back({{"id",item.id},{"quantity",item.quantity}});
    return result;
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
    return auth.compare(0,7,"Bearer ") == 0 ? auth.substr(7) : "";
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
        {".ico","image/x-icon"},{".woff2","font/woff2"}};
    auto type = types.find(extension);
    if (type == types.end()) return error(404,"File not found");
    std::ifstream input("../../frontend" + relative, std::ios::binary);
    if (!input) return error(404,"File not found");
    HttpResponse result;
    result.contentType = type->second;
    result.body.assign(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
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
    if (request.method == "POST" && (request.path == "/api/register" || request.path == "/api/login")) {
        auto fields = json::object(request.body);
        auto email = stringField(fields,"email",254);
        auto password = stringField(fields,"password",1024);
        UserRow user;
        if (request.path == "/api/register") {
            if (password.size() < 6 || email.find('@') == std::string::npos) return error(400,"Invalid email or password");
            if (db.emailExists(email)) return error(409,"Email already registered");
            db.createUser(email,password,user);
        } else {
            int retry = authFailures.retryAfter();
            if (retry) {
                auto result = error(429,"Too many failed logins; try again later");
                result.extraHeaders["Retry-After"] = std::to_string(retry);
                return result;
            }
            if (!db.verifyLogin(email,password,user)) {
                authFailures.record();
                return error(401,"Invalid credentials");
            }
            db.touchLastLogin(user.id);
        }
        return response(request.path == "/api/register" ? 201 : 200,
            {{"token",db.createSession(user.id)},{"user",userJson(user)}});
    }
    long long userId = 0;
    const auto token = tokenFor(request);
    if (!db.userIdForToken(token,userId)) return error(401,"Authentication required");
    if (request.method == "POST" && request.path == "/api/logout") {
        db.revokeSession(token); return response(200,{{"ok",true}});
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
        int quantity = integerField(fields,"quantity",1000);
        int status = db.purchase(userId,itemId,quantity);
        if (status != 200) return error(status,"Unknown item, insufficient coins, or inventory limit");
        UserRow user; db.getUserById(userId,user);
        return response(200,{{"user",userJson(user)},{"inventory",inventoryJson(db,userId)}});
    }
    if (request.method == "POST" && request.path == "/api/pet/feed") {
        auto fields = json::object(request.body);
        int itemId = integerField(fields,"itemId",2147483647);
        int status = db.feed(userId,itemId);
        if (status != 200) return error(status, status == 400 ? "Item is not food"
            : status == 409 ? "Food not owned" : "Item or pet not found");
        UserRow user; PetRow pet; db.getUserById(userId,user); db.getPet(userId,pet);
        return response(200,{{"user",userJson(user)},{"pet",petJson(pet)},
            {"inventory",inventoryJson(db,userId)}});
    }
    if (request.method == "PUT" && request.path == "/api/pet") {
        auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("name")) return error(400,"Only name may be updated");
        auto name = stringField(fields,"name",80);
        if (!db.renamePet(userId,name)) return error(404,"Pet not found");
        UserRow user; PetRow pet; db.getUserById(userId,user); db.getPet(userId,pet);
        return response(200,{{"user",userJson(user)},{"pet",petJson(pet)}});
    }
    if (request.method == "PUT" && request.path == "/api/pet/species") {
        auto fields = json::object(request.body);
        if (fields.size() != 1 || !fields.count("species")) return error(400,"Only species may be updated");
        auto species = stringField(fields,"species",8);
        if (species != "cat" && species != "dog") return error(400,"Species must be cat or dog");
        if (!db.setPetSpecies(userId,species)) return error(404,"Pet not found");
        UserRow user; PetRow pet; db.getUserById(userId,user); db.getPet(userId,pet);
        return response(200,{{"user",userJson(user)},{"pet",petJson(pet)}});
    }
    const std::string prefix = "/api/exercises/", suffix = "/submissions";
    const auto& path = request.path;
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
        if (!db.open(argc >= 3 ? argv[2] : "virtual_pet.db") || !db.runSchema() || !db.seedExercisesIfEmpty())
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
