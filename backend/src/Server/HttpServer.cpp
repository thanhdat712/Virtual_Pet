#include "HttpServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace {

std::string toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// Đọc toàn bộ cho tới khi gặp "\r\n\r\n" (hết header) rồi đọc thêm theo Content-Length.
int receiveRequest(SOCKET client, std::string& raw) {
    char buffer[4096];
    const size_t maxHeader = 16384, maxBody = 32768;
    size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        int n = recv(client, buffer, sizeof(buffer), 0);
        if (n <= 0) return 400;
        raw.append(buffer, n);
        headerEnd = raw.find("\r\n\r\n");
        if ((headerEnd == std::string::npos && raw.size() > maxHeader) ||
            (headerEnd != std::string::npos && headerEnd > maxHeader))
            return 431;
    }
    size_t contentLength = 0;
    std::istringstream headers(raw.substr(0,headerEnd));
    std::string line, method, target, version, extra;
    std::getline(headers,line);
    std::istringstream first(line);
    if (!(first >> method >> target >> version) || first >> extra || version != "HTTP/1.1" || target.empty() || target[0] != '/') return 400;
    bool lengthSeen = false, hostSeen = false;
    while (std::getline(headers,line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == std::string::npos || colon == 0) return 400;
        auto key = toLower(line.substr(0,colon));
        if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") != std::string::npos) return 400;
        auto value = line.substr(colon+1);
        auto begin = value.find_first_not_of(" \t");
        value = begin == std::string::npos ? "" : value.substr(begin);
        auto end = value.find_last_not_of(" \t");
        if (end != std::string::npos) value.resize(end+1);
        if (key == "transfer-encoding") return 400;
        if (key == "host") { if (hostSeen || value.empty()) return 400; hostSeen = true; }
        if (key == "content-length") {
            if (lengthSeen || value.empty()) return 400;
            lengthSeen = true;
            for (char digit : value) {
                if (digit < '0' || digit > '9') return 400;
                contentLength = contentLength*10 + static_cast<size_t>(digit-'0');
                if (contentLength > maxBody) return 413;
            }
        }
    }
    if (!hostSeen) return 400;
    size_t bodyStart = headerEnd + 4;
    size_t have = raw.size() - bodyStart;
    if (have > contentLength) return 400;
    while (have < contentLength) {
        int n = recv(client, buffer, static_cast<int>(std::min(sizeof(buffer),contentLength-have)), 0);
        if (n <= 0) return 400;
        raw.append(buffer, n);
        have = raw.size() - bodyStart;
    }
    return 200;
}

HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    size_t headerEnd = raw.find("\r\n\r\n");
    std::string headerPart = headerEnd == std::string::npos ? raw : raw.substr(0, headerEnd);
    if (headerEnd != std::string::npos) req.body = raw.substr(headerEnd + 4);

    size_t lineEnd = headerPart.find("\r\n");
    std::string requestLine = lineEnd == std::string::npos ? headerPart : headerPart.substr(0, lineEnd);

    // "METHOD PATH?QUERY HTTP/1.1"
    size_t sp1 = requestLine.find(' ');
    size_t sp2 = requestLine.find(' ', sp1 + 1);
    if (sp1 != std::string::npos && sp2 != std::string::npos) {
        req.method = requestLine.substr(0, sp1);
        std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
        size_t q = target.find('?');
        if (q == std::string::npos) {
            req.path = target;
        } else {
            req.path = target.substr(0, q);
            req.query = target.substr(q + 1);
        }
    }

    // Header còn lại.
    size_t pos = lineEnd == std::string::npos ? std::string::npos : lineEnd + 2;
    while (pos != std::string::npos && pos < headerPart.size()) {
        size_t next = headerPart.find("\r\n", pos);
        std::string line = next == std::string::npos ? headerPart.substr(pos)
                                                      : headerPart.substr(pos, next - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = toLower(line.substr(0, colon));
            size_t vstart = colon + 1;
            while (vstart < line.size() && (line[vstart] == ' ' || line[vstart] == '\t')) ++vstart;
            auto value = line.substr(vstart);
            auto last = value.find_last_not_of(" \t");
            req.headers[key] = last == std::string::npos ? "" : value.substr(0,last+1);
        }
        if (next == std::string::npos) break;
        pos = next + 2;
    }
    return req;
}

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

void sendResponse(SOCKET client, const HttpResponse& res) {
    std::string out;
    out.reserve(res.body.size() + 256);
    out += "HTTP/1.1 ";
    out += std::to_string(res.status);
    out += ' ';
    out += statusText(res.status);
    out += "\r\n";
    out += "Content-Type: " + res.contentType + "\r\n";
    out += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
    out += "Connection: close\r\n";
    for (const auto& kv : res.extraHeaders) {
        out += kv.first + ": " + kv.second + "\r\n";
    }
    out += "\r\n";
    out += res.body;

    size_t sent = 0;
    while (sent < out.size()) {
        int n = send(client, out.data() + sent, static_cast<int>(out.size() - sent), 0);
        if (n <= 0) break;
        sent += n;
    }
}

}  // namespace

HttpServer::HttpServer() {}

HttpServer::~HttpServer() {
    if (wsaStarted_) WSACleanup();
}

bool HttpServer::listen(const std::string& host, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        lastError_ = "WSAStartup that bai";
        return false;
    }
    wsaStarted_ = true;

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        lastError_ = "Khong tao duoc socket";
        return false;
    }

    BOOL exclusive = TRUE;
    setsockopt(listenSock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        lastError_ = "Bind that bai (cong co the dang duoc dung)";
        closesocket(listenSock);
        return false;
    }
    if (::listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        lastError_ = "Listen that bai";
        closesocket(listenSock);
        return false;
    }

    std::printf("API dang chay tai http://%s:%d\n", host.c_str(), port);
    std::fflush(stdout);

    while (true) {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        DWORD timeout = 3000;
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));

        std::string raw;
        int received = receiveRequest(client, raw);
        if (received == 200) {
            HttpRequest req = parseRequest(raw);
            HttpResponse res;
            if (handler_) {
                res = handler_(req);
            } else {
                res.status = 500;
                res.body = "{\"error\":\"khong co handler\"}";
            }
            sendResponse(client, res);
        } else {
            HttpResponse res;
            res.status = received;
            res.body = "{\"error\":\"Invalid or oversized HTTP request\"}";
            sendResponse(client,res);
        }
        closesocket(client);
    }

    closesocket(listenSock);
    return true;
}
