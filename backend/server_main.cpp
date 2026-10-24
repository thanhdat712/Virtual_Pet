#include "AuthService.h"
#include "Game.h"
#include "GameRepository.h"
#include "Decoration.h"
#include "Food.h"
#include "Shop.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <mutex>
#include <utility>

#ifndef PET_FRONTEND_DIR
#define PET_FRONTEND_DIR "frontend"
#endif

namespace {
constexpr const char* SESSION_COOKIE = "vp_session";
constexpr std::int64_t SESSION_LIFETIME_SECONDS = 7 * 24 * 60 * 60;

bool acceptsJson(const httplib::Request& request) {
    std::string contentType = request.get_header_value("Content-Type");
    std::transform(contentType.begin(), contentType.end(), contentType.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return contentType.rfind("application/json", 0) == 0
        && (contentType.size() == 16 || contentType[16] == ';' || contentType[16] == ' ');
}

void setJson(httplib::Response& response, int status, const nlohmann::json& body) {
    response.status = status;
    response.set_header("Cache-Control", "no-store");
    response.set_content(body.dump(), "application/json; charset=utf-8");
}

std::string sessionCookie(const httplib::Request& request) {
    const std::string cookies = request.get_header_value("Cookie");
    std::size_t start = 0;
    while (start < cookies.size()) {
        const std::size_t end = cookies.find(';', start);
        const std::string item = cookies.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        const std::size_t equals = item.find('=');
        if (equals != std::string::npos) {
            std::size_t keyStart = 0;
            std::size_t keyEnd = equals;
            while (keyStart < keyEnd && std::isspace(static_cast<unsigned char>(item[keyStart]))) {
                ++keyStart;
            }
            while (keyEnd > keyStart && std::isspace(static_cast<unsigned char>(item[keyEnd - 1]))) {
                --keyEnd;
            }
            if (item.compare(keyStart, keyEnd - keyStart, SESSION_COOKIE) == 0) {
                std::size_t valueStart = equals + 1;
                while (valueStart < item.size() && std::isspace(static_cast<unsigned char>(item[valueStart]))) {
                    ++valueStart;
                }
                return item.substr(valueStart);
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return {};
}

bool parseRegistrationCredentials(const httplib::Request& request, std::string& username,
                                  std::string& email, std::string& password) {
    if (!acceptsJson(request)) return false;
    try {
        const nlohmann::json payload = nlohmann::json::parse(request.body);
        if (!payload.is_object()
            || !payload.contains("username") || !payload["username"].is_string()
            || !payload.contains("email") || !payload["email"].is_string()
            || !payload.contains("password") || !payload["password"].is_string()) return false;
        username = payload["username"].get<std::string>();
        email = payload["email"].get<std::string>();
        password = payload["password"].get<std::string>();
        return !email.empty();
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool parseEmailCredentials(const httplib::Request& request, std::string& email, std::string& password) {
    if (!acceptsJson(request)) return false;
    try {
        const nlohmann::json payload = nlohmann::json::parse(request.body);
        if (!payload.is_object()
            || !payload.contains("email") || !payload["email"].is_string()
            || !payload.contains("password") || !payload["password"].is_string()) return false;
        email = payload["email"].get<std::string>();
        password = payload["password"].get<std::string>();
        return !email.empty();
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool positiveInteger(const nlohmann::json& payload, const char* field, int maximum, int& value) {
    if (!payload.is_object() || !payload.contains(field) || !payload[field].is_number_integer()) return false;
    try {
        const auto candidate = payload[field].get<std::int64_t>();
        if (candidate < 1 || candidate > maximum) return false;
        value = static_cast<int>(candidate);
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

int statusForAction(ActionCode code) {
    switch (code) {
        case ActionCode::ITEM_NOT_FOUND: return 404;
        case ActionCode::INVALID_ARGUMENT:
        case ActionCode::NOT_FOOD:
        case ActionCode::NOT_DECORATION: return 400;
        case ActionCode::OK: return 200;
        case ActionCode::INVALID_SNAPSHOT: return 500;
        default: return 409;
    }
}

const char* slotName(DecorationSlot slot) {
    switch (slot) {
        case DecorationSlot::HAT: return "headwear";
        case DecorationSlot::BACKGROUND: return "background";
        case DecorationSlot::FLOOR: return "floor";
    }
    return "unknown";
}

nlohmann::json catalogJson(const Game& game) {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& item : game.getShop().getItems()) {
        nlohmann::json entry = {{"id", item->getId()}, {"name", item->getName()},
            {"description", item->getDescription()}, {"price", item->getPrice()}, {"image", item->getImagePath()}};
        if (item->getType() == ItemType::FOOD) {
            const auto food = std::dynamic_pointer_cast<Food>(item);
            entry["category"] = "food";
            entry["filter"] = "food";
            entry["hungerRestore"] = food->getHungerRestore();
            entry["happinessRestore"] = food->getHappinessRestore();
        } else {
            entry["category"] = "decoration";
            entry["filter"] = slotName(std::dynamic_pointer_cast<Decoration>(item)->getSlot());
        }
        result.push_back(std::move(entry));
    }
    return result;
}

nlohmann::json stateJson(const Game& game, int userId, const std::string& username) {
    const auto snapshot = game.exportSnapshot();
    nlohmann::json inventory = nlohmann::json::array();
    for (const auto& item : snapshot.inventory) inventory.push_back({{"itemId", item.itemId}, {"quantity", item.quantity}});
    nlohmann::json equipment = {{"headwear", nullptr}, {"background", nullptr}, {"floor", nullptr}};
    for (const auto& item : snapshot.equipment) equipment[slotName(item.slot)] = item.itemId;
    return {{"user", {{"id", userId}, {"username", username}, {"coins", snapshot.coins}}},
        {"pet", {{"id", snapshot.pet.id}, {"name", snapshot.pet.name}, {"level", snapshot.pet.level},
            {"health", snapshot.pet.health}, {"experience", 0}, {"happiness", snapshot.pet.happiness},
            {"hunger", snapshot.pet.hunger}, {"energy", snapshot.pet.energy}}},
        {"inventory", inventory}, {"equipment", equipment}};
}

bool parseJsonObject(const httplib::Request& request, nlohmann::json& payload) {
    if (!acceptsJson(request)) return false;
    try {
        payload = nlohmann::json::parse(request.body);
        return payload.is_object();
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool parseDecorationSlot(const nlohmann::json& payload, DecorationSlot& slot) {
    if (!payload.is_object() || !payload.contains("slot") || !payload["slot"].is_string()) return false;
    const std::string value = payload["slot"].get<std::string>();
    if (value == "headwear") slot = DecorationSlot::HAT;
    else if (value == "background") slot = DecorationSlot::BACKGROUND;
    else if (value == "floor") slot = DecorationSlot::FLOOR;
    else return false;
    return true;
}

class GameApi {
public:
    explicit GameApi(const std::string& databasePath) : repository_(database_) {
        if (!database_.open(databasePath) || !database_.initializeSchema())
            throw std::runtime_error("Could not initialize game data: " + database_.getLastError());
    }

    nlohmann::json catalog() { Game game; return catalogJson(game); }

    template <typename Action>
    nlohmann::json run(int userId, const std::string& username, Action action, ActionResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto snapshot = repository_.loadGame(userId);
        if (!snapshot) {
            if (!repository_.createInitialGame(userId, username)) throw std::runtime_error("Could not create game data.");
            snapshot = repository_.loadGame(userId);
        }
        if (!snapshot) throw std::runtime_error("Could not load game data.");
        Game game;
        result = game.importSnapshot(*snapshot);
        if (!result.ok()) return {};
        result = game.start();
        if (!result.ok()) return {};
        result = action(game);
        if (!result.ok()) return {};
        if (!repository_.saveGame(game.exportSnapshot())) throw std::runtime_error("Could not save game data.");
        return stateJson(game, userId, username);
    }

private:
    DatabaseConnection database_;
    GameRepository repository_;
    std::mutex mutex_;
};
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        try {
            const int requestedPort = std::stoi(argv[1]);
            if (requestedPort < 1 || requestedPort > 65535) {
                std::cerr << "Port must be between 1 and 65535.\n";
                return 1;
            }
            port = requestedPort;
        } catch (const std::exception&) {
            std::cerr << "Usage: Pet_Raising_Server [port] [database-path]\n";
            return 1;
        }
    }

    const std::string databasePath = argc > 2 ? argv[2] : "backend/database/virtual_pet.db";

    try {
        AuthService authService(databasePath);
        GameApi gameApi(databasePath);
        httplib::Server server;
        server.set_payload_max_length(4096);

        server.Post("/api/auth/register", [&authService](const httplib::Request& request, httplib::Response& response) {
            if (!acceptsJson(request)) {
                setJson(response, 415, {
                    {"success", false},
                    {"code", "UNSUPPORTED_MEDIA_TYPE"},
                    {"message", "Yêu cầu gửi JSON."}
                });
                return;
            }

            std::string username;
            std::string password;
            std::string email;
            if (!parseRegistrationCredentials(request, username, email, password)) {
                setJson(response, 400, {
                    {"success", false},
                    {"code", "INVALID_ARGUMENT"},
                    {"message", "Enter username, email, and password."}
                });
                return;
            }

            const RegisterResult result = authService.registerUser(username, std::move(email), std::move(password));

            nlohmann::json body;
            switch (result.status) {
                case RegisterStatus::SUCCESS:
                    response.status = 201;
                    body = {
                        {"success", true},
                        {"code", "REGISTERED"},
                        {"message", "Tạo tài khoản thành công."},
                        {"user", {{"id", result.userId}, {"username", username}}}
                    };
                    break;
                case RegisterStatus::USERNAME_TAKEN:
                    response.status = 409;
                    body = {
                        {"success", false},
                        {"code", "USERNAME_TAKEN"},
                        {"message", "Tên người dùng này đã được sử dụng."}
                    };
                    break;
                case RegisterStatus::EMAIL_TAKEN:
                    response.status = 409;
                    body = {
                        {"success", false},
                        {"code", "EMAIL_TAKEN"},
                        {"message", "This email is already registered."}
                    };
                    break;
                case RegisterStatus::INVALID_USERNAME:
                    response.status = 400;
                    body = {
                        {"success", false},
                        {"code", "INVALID_USERNAME"},
                        {"message", "Tên người dùng cần có 3–20 ký tự: chữ cái không dấu, số hoặc dấu gạch dưới."}
                    };
                    break;
                case RegisterStatus::INVALID_EMAIL:
                    response.status = 400;
                    body = {
                        {"success", false},
                        {"code", "INVALID_EMAIL"},
                        {"message", "Enter an email address no longer than 254 characters."}
                    };
                    break;
                case RegisterStatus::INVALID_PASSWORD:
                    response.status = 400;
                    body = {
                        {"success", false},
                        {"code", "INVALID_PASSWORD"},
                        {"message", "Mật khẩu cần có ít nhất 8 ký tự hợp lệ."}
                    };
                    break;
                case RegisterStatus::HASH_ERROR:
                    std::cerr << "Registration password hashing error: " << result.detail << '\n';
                    response.status = 500;
                    body = {
                        {"success", false},
                        {"code", "REGISTRATION_FAILED"},
                        {"message", "Không thể bảo vệ mật khẩu lúc này. Vui lòng thử lại."}
                    };
                    break;
                case RegisterStatus::DATABASE_ERROR:
                    std::cerr << "Registration database error: " << result.detail << '\n';
                    response.status = 500;
                    body = {
                        {"success", false},
                        {"code", "REGISTRATION_FAILED"},
                        {"message", "Chưa thể tạo tài khoản lúc này. Vui lòng thử lại."}
                    };
                    break;
            }

            response.set_header("Cache-Control", "no-store");
            response.set_content(body.dump(), "application/json; charset=utf-8");
        });

        server.Post("/api/auth/login", [&authService](const httplib::Request& request, httplib::Response& response) {
            std::string email;
            std::string password;
            if (!acceptsJson(request)) {
                setJson(response, 415, {
                    {"success", false},
                    {"code", "UNSUPPORTED_MEDIA_TYPE"},
                    {"message", "Yêu cầu gửi JSON."}
                });
                return;
            }
            if (!parseEmailCredentials(request, email, password)) {
                setJson(response, 400, {
                    {"success", false},
                    {"code", "INVALID_ARGUMENT"},
                    {"message", "Enter your email and password."}
                });
                return;
            }

            const LoginResult result = authService.loginByEmail(std::move(email), std::move(password));
            if (result.status == LoginStatus::SUCCESS) {
                response.set_header(
                    "Set-Cookie",
                    std::string(SESSION_COOKIE) + "=" + result.sessionToken
                        + "; Path=/; HttpOnly; SameSite=Strict; Max-Age="
                        + std::to_string(SESSION_LIFETIME_SECONDS));
                setJson(response, 200, {
                    {"success", true},
                    {"code", "AUTHENTICATED"},
                    {"message", "Đăng nhập thành công."},
                    {"user", {{"id", result.userId}, {"username", result.username}}}
                });
                return;
            }
            if (result.status == LoginStatus::RATE_LIMITED) {
                response.set_header("Retry-After", "300");
                setJson(response, 429, {
                    {"success", false},
                    {"code", "RATE_LIMITED"},
                    {"message", "Bạn đã thử đăng nhập quá nhiều lần. Vui lòng thử lại sau."}
                });
                return;
            }
            if (result.status == LoginStatus::INVALID_CREDENTIALS) {
                setJson(response, 401, {
                    {"success", false},
                    {"code", "INVALID_CREDENTIALS"},
                    {"message", "Sai tên đăng nhập hoặc mật khẩu."}
                });
                return;
            }

            std::cerr << "Login failed due to an internal error: " << result.detail << '\n';
            setJson(response, 500, {
                {"success", false},
                {"code", "LOGIN_FAILED"},
                {"message", "Chưa thể đăng nhập lúc này. Vui lòng thử lại."}
            });
        });

        server.Get("/api/auth/session", [&authService](const httplib::Request& request, httplib::Response& response) {
            const SessionResult session = authService.currentSession(sessionCookie(request));
            if (!session.detail.empty()) {
                std::cerr << "Could not check login session: " << session.detail << '\n';
                setJson(response, 500, {
                    {"success", false},
                    {"code", "INTERNAL_ERROR"},
                    {"message", "Chưa thể kiểm tra phiên đăng nhập."}
                });
                return;
            }
            setJson(response, 200, {
                {"success", true},
                {"authenticated", session.authenticated},
                {"user", session.authenticated
                    ? nlohmann::json{{"id", session.userId}, {"username", session.username}}
                    : nlohmann::json(nullptr)}
            });
        });

        server.Post("/api/auth/logout", [&authService](const httplib::Request& request, httplib::Response& response) {
            std::string detail;
            if (authService.logout(sessionCookie(request), &detail) != LogoutStatus::SUCCESS) {
                std::cerr << "Logout failed: " << detail << '\n';
                setJson(response, 500, {
                    {"success", false},
                    {"code", "INTERNAL_ERROR"},
                    {"message", "Chưa thể đăng xuất lúc này. Vui lòng thử lại."}
                });
                return;
            }
            response.set_header(
                "Set-Cookie",
                std::string(SESSION_COOKIE) + "=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
            setJson(response, 200, {
                {"success", true},
                {"code", "LOGGED_OUT"},
                {"message", "Đã đăng xuất."}
            });
        });

        const auto requireSession = [&authService](
            const httplib::Request& request, httplib::Response& response) {
            const SessionResult session = authService.currentSession(sessionCookie(request));
            if (!session.detail.empty()) {
                std::cerr << "Could not check game session: " << session.detail << '\n';
                setJson(response, 500, {
                    {"success", false},
                    {"code", "INTERNAL_ERROR"},
                    {"message", "Chưa thể kiểm tra phiên đăng nhập."}
                });
                return SessionResult{};
            }
            if (!session.authenticated) {
                setJson(response, 401, {
                    {"success", false},
                    {"code", "UNAUTHENTICATED"},
                    {"message", "Vui lòng đăng nhập để tiếp tục."}
                });
                return SessionResult{};
            }
            return session;
        };

        const auto runAction = [&gameApi, &requireSession](
            const httplib::Request& request,
            httplib::Response& response,
            const std::function<ActionResult(Game&)>& action) {
            const SessionResult session = requireSession(request, response);
            if (!session.authenticated) return;

            ActionResult result;
            const nlohmann::json state = gameApi.run(
                session.userId, session.username, action, result);
            if (!result.ok()) {
                const bool internalError = result.code == ActionCode::INVALID_SNAPSHOT;
                setJson(response, internalError ? 500 : statusForAction(result.code), {
                    {"success", false},
                    {"code", internalError ? "INTERNAL_ERROR" : toString(result.code)},
                    {"message", internalError ? "Dữ liệu trò chơi không hợp lệ." : result.message}
                });
                return;
            }
            setJson(response, 200, {
                {"success", true},
                {"code", "OK"},
                {"message", result.message},
                {"levelsGained", result.levelsGained},
                {"state", state}
            });
        };

        server.Get("/api/shop", [&gameApi](const httplib::Request&, httplib::Response& response) {
            setJson(response, 200, gameApi.catalog());
        });

        server.Get("/api/game/state", [&gameApi, &requireSession](
            const httplib::Request& request, httplib::Response& response) {
            const SessionResult session = requireSession(request, response);
            if (!session.authenticated) return;

            ActionResult result;
            const nlohmann::json state = gameApi.run(
                session.userId, session.username,
                [](Game&) { return ActionResult::success("Game state loaded."); }, result);
            if (!result.ok()) {
                setJson(response, 500, {
                    {"success", false},
                    {"code", "INTERNAL_ERROR"},
                    {"message", "Không thể tải dữ liệu trò chơi."}
                });
                return;
            }
            setJson(response, 200, state);
        });

        server.Post("/api/shop/purchases", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            nlohmann::json payload;
            int itemId = 0;
            int quantity = 0;
            if (!parseJsonObject(request, payload)
                || !positiveInteger(payload, "itemId", std::numeric_limits<int>::max(), itemId)
                || !positiveInteger(payload, "quantity", 999, quantity)) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "itemId and quantity must be valid positive integers."}
                });
                return;
            }
            runAction(request, response, [itemId, quantity](Game& game) {
                return game.buy(itemId, quantity);
            });
        });

        server.Post("/api/pet/feed", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            nlohmann::json payload;
            int itemId = 0;
            if (!parseJsonObject(request, payload)
                || !positiveInteger(payload, "itemId", std::numeric_limits<int>::max(), itemId)) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "itemId must be a valid positive integer."}
                });
                return;
            }
            runAction(request, response, [itemId](Game& game) { return game.feed(itemId); });
        });

        server.Post("/api/pet/play", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            if (!request.body.empty()) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "This action does not accept a request body."}
                });
                return;
            }
            runAction(request, response, [](Game& game) { return game.play(); });
        });

        server.Post("/api/pet/sleep", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            if (!request.body.empty()) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "This action does not accept a request body."}
                });
                return;
            }
            runAction(request, response, [](Game& game) { return game.sleep(); });
        });

        server.Post("/api/pet/bath", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            if (!request.body.empty()) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "This action does not accept a request body."}
                });
                return;
            }
            runAction(request, response, [](Game& game) { return game.bath(); });
        });

        server.Post("/api/pet/decorate", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            nlohmann::json payload;
            int itemId = 0;
            if (!parseJsonObject(request, payload)
                || !positiveInteger(payload, "itemId", std::numeric_limits<int>::max(), itemId)) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "itemId must be a valid positive integer."}
                });
                return;
            }
            runAction(request, response, [itemId](Game& game) { return game.decorate(itemId); });
        });

        server.Post("/api/pet/unequip", [&runAction](
            const httplib::Request& request, httplib::Response& response) {
            nlohmann::json payload;
            DecorationSlot slot;
            if (!parseJsonObject(request, payload) || !parseDecorationSlot(payload, slot)) {
                setJson(response, 400, {
                    {"success", false}, {"code", "INVALID_ARGUMENT"},
                    {"message", "slot must be headwear, background, or floor."}
                });
                return;
            }
            runAction(request, response, [slot](Game& game) {
                return game.unequipDecoration(slot);
            });
        });

        server.set_error_handler([](const httplib::Request& request, httplib::Response& response) {
            if (request.path.rfind("/api/", 0) == 0 && response.body.empty()) {
                setJson(response, 404, {
                    {"success", false},
                    {"code", "NOT_FOUND"},
                    {"message", "Không tìm thấy API."}
                });
            }
        });

        server.set_exception_handler([](
            const httplib::Request&,
            httplib::Response& response,
            std::exception_ptr error) {
            try {
                if (error) {
                    std::rethrow_exception(error);
                }
            } catch (const std::exception& failure) {
                std::cerr << "HTTP request failed: " << failure.what() << '\n';
            }
            setJson(response, 500, {
                {"success", false},
                {"code", "INTERNAL_ERROR"},
                {"message", "Đã xảy ra lỗi máy chủ."}
            });
        });

        if (!server.set_mount_point("/", PET_FRONTEND_DIR)) {
            std::cerr << "Could not serve frontend directory: " << PET_FRONTEND_DIR << '\n';
            return 1;
        }

        std::cout << "Virtual Pet is running at http://127.0.0.1:" << port << "/\n"
                  << "API: POST /api/auth/register, POST /api/auth/login, "
                     "POST /api/auth/logout, GET /api/auth/session, GET /api/game/state, "
                     "GET /api/shop, POST /api/shop/purchases, POST /api/pet/*\n"
                  << "Database: " << databasePath << "\n";
        if (!server.listen("127.0.0.1", port)) {
            std::cerr << "Could not start the HTTP server on port " << port << ".\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "Could not start Virtual Pet server: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
