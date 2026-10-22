#include "AuthService.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
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

bool parseCredentials(const httplib::Request& request, std::string& username, std::string& password) {
    if (!acceptsJson(request)) {
        return false;
    }
    try {
        const nlohmann::json payload = nlohmann::json::parse(request.body);
        if (!payload.is_object()
            || !payload.contains("username")
            || !payload["username"].is_string()
            || !payload.contains("password")
            || !payload["password"].is_string()) {
            return false;
        }
        username = payload["username"].get<std::string>();
        password = payload["password"].get<std::string>();
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}
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
        httplib::Server server;
        server.set_payload_max_length(4096);

        server.Post("/api/auth/register", [&authService](const httplib::Request& request, httplib::Response& response) {
            if (!acceptsJson(request)) {
                response.status = 415;
                response.set_content(
                    R"({"success":false,"code":"unsupported_media_type","message":"Yêu cầu gửi JSON."})",
                    "application/json; charset=utf-8");
                return;
            }

            std::string username;
            std::string password;
            if (!parseCredentials(request, username, password)) {
                response.status = 400;
                response.set_content(
                    R"({"success":false,"code":"invalid_request","message":"Cần nhập tên người dùng và mật khẩu."})",
                    "application/json; charset=utf-8");
                return;
            }

            const RegisterResult result = authService.registerUser(username, std::move(password));

            nlohmann::json body;
            switch (result.status) {
                case RegisterStatus::SUCCESS:
                    response.status = 201;
                    body = {
                        {"success", true},
                        {"code", "registered"},
                        {"message", "Tạo tài khoản thành công."},
                        {"user", {{"id", result.userId}, {"username", username}}}
                    };
                    break;
                case RegisterStatus::USERNAME_TAKEN:
                    response.status = 409;
                    body = {
                        {"success", false},
                        {"code", "username_taken"},
                        {"message", "Tên người dùng này đã được sử dụng."}
                    };
                    break;
                case RegisterStatus::INVALID_USERNAME:
                    response.status = 400;
                    body = {
                        {"success", false},
                        {"code", "invalid_username"},
                        {"message", "Tên người dùng cần có 3–20 ký tự: chữ cái không dấu, số hoặc dấu gạch dưới."}
                    };
                    break;
                case RegisterStatus::INVALID_PASSWORD:
                    response.status = 400;
                    body = {
                        {"success", false},
                        {"code", "invalid_password"},
                        {"message", "Mật khẩu cần có ít nhất 8 ký tự hợp lệ."}
                    };
                    break;
                case RegisterStatus::HASH_ERROR:
                    std::cerr << "Registration password hashing error: " << result.detail << '\n';
                    response.status = 500;
                    body = {
                        {"success", false},
                        {"code", "registration_failed"},
                        {"message", "Không thể bảo vệ mật khẩu lúc này. Vui lòng thử lại."}
                    };
                    break;
                case RegisterStatus::DATABASE_ERROR:
                    std::cerr << "Registration database error: " << result.detail << '\n';
                    response.status = 500;
                    body = {
                        {"success", false},
                        {"code", "registration_failed"},
                        {"message", "Chưa thể tạo tài khoản lúc này. Vui lòng thử lại."}
                    };
                    break;
            }

            response.set_content(body.dump(), "application/json; charset=utf-8");
        });

        server.Post("/api/auth/login", [&authService](const httplib::Request& request, httplib::Response& response) {
            std::string username;
            std::string password;
            if (!acceptsJson(request)) {
                setJson(response, 415, {
                    {"success", false},
                    {"code", "unsupported_media_type"},
                    {"message", "Yêu cầu gửi JSON."}
                });
                return;
            }
            if (!parseCredentials(request, username, password)) {
                setJson(response, 400, {
                    {"success", false},
                    {"code", "invalid_request"},
                    {"message", "Cần nhập tên người dùng và mật khẩu."}
                });
                return;
            }

            const LoginResult result = authService.login(std::move(username), std::move(password));
            if (result.status == LoginStatus::SUCCESS) {
                response.set_header(
                    "Set-Cookie",
                    std::string(SESSION_COOKIE) + "=" + result.sessionToken
                        + "; Path=/; HttpOnly; SameSite=Strict; Max-Age="
                        + std::to_string(SESSION_LIFETIME_SECONDS));
                setJson(response, 200, {
                    {"success", true},
                    {"code", "authenticated"},
                    {"message", "Đăng nhập thành công."},
                    {"user", {{"id", result.userId}, {"username", result.username}}}
                });
                return;
            }
            if (result.status == LoginStatus::INVALID_CREDENTIALS || result.status == LoginStatus::RATE_LIMITED) {
                setJson(response, 401, {
                    {"success", false},
                    {"code", "invalid_credentials"},
                    {"message", "Sai tên đăng nhập hoặc mật khẩu."}
                });
                return;
            }

            std::cerr << "Login failed due to an internal error: " << result.detail << '\n';
            setJson(response, 500, {
                {"success", false},
                {"code", "login_failed"},
                {"message", "Chưa thể đăng nhập lúc này. Vui lòng thử lại."}
            });
        });

        server.Get("/api/auth/session", [&authService](const httplib::Request& request, httplib::Response& response) {
            const SessionResult session = authService.currentSession(sessionCookie(request));
            if (!session.detail.empty()) {
                std::cerr << "Could not check login session: " << session.detail << '\n';
                setJson(response, 500, {
                    {"success", false},
                    {"code", "session_check_failed"},
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
                    {"code", "logout_failed"},
                    {"message", "Chưa thể đăng xuất lúc này. Vui lòng thử lại."}
                });
                return;
            }
            response.set_header(
                "Set-Cookie",
                std::string(SESSION_COOKIE) + "=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
            setJson(response, 200, {
                {"success", true},
                {"code", "logged_out"},
                {"message", "Đã đăng xuất."}
            });
        });

        server.set_error_handler([](const httplib::Request& request, httplib::Response& response) {
            if (request.path.rfind("/api/", 0) == 0 && response.body.empty()) {
                response.status = 404;
                response.set_content(
                    R"({"success":false,"code":"not_found","message":"Không tìm thấy API."})",
                    "application/json; charset=utf-8");
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
            response.status = 500;
            response.set_content(
                R"({"success":false,"code":"internal_error","message":"Đã xảy ra lỗi máy chủ."})",
                "application/json; charset=utf-8");
        });

        if (!server.set_mount_point("/", PET_FRONTEND_DIR)) {
            std::cerr << "Could not serve frontend directory: " << PET_FRONTEND_DIR << '\n';
            return 1;
        }

        std::cout << "Virtual Pet is running at http://127.0.0.1:" << port << "/\n"
                  << "Auth endpoints: POST /api/auth/register, POST /api/auth/login, "
                     "POST /api/auth/logout, GET /api/auth/session\n"
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
