#ifndef SERVER_HTTP_SERVER_H
#define SERVER_HTTP_SERVER_H

#include <functional>
#include <map>
#include <string>

// Yêu cầu HTTP đã phân tích ở mức tối giản.
struct HttpRequest {
    std::string method;
    std::string path;                            // phần đường dẫn, không gồm query
    std::string query;                           // phần sau dấu '?'
    std::map<std::string, std::string> headers;  // key viết thường
    std::string body;

    std::string header(const std::string& name) const {
        auto it = headers.find(name);
        return it == headers.end() ? std::string() : it->second;
    }
};

// Phản hồi HTTP.
struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::map<std::string, std::string> extraHeaders;
};

// HTTP server tối giản dùng Winsock, chạy đơn luồng (mỗi lần 1 request).
// Phù hợp cho môi trường phát triển cục bộ.
class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServer();
    ~HttpServer();

    void setHandler(Handler handler) { handler_ = handler; }
    // Khởi động server; vòng lặp chặn cho tới khi bị dừng. Trả về false nếu lỗi khởi tạo.
    bool listen(const std::string& host, int port);

    std::string lastError() const { return lastError_; }

private:
    Handler handler_;
    std::string lastError_;
    bool wsaStarted_ = false;
};

#endif
