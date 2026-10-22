#ifndef SERVER_JSON_H
#define SERVER_JSON_H

#include "../../third_party/nlohmann/json.hpp"
#include <stdexcept>
#include <string>

namespace json {

using Value = nlohmann::json;

inline Value object(const std::string& body) {
    auto value = Value::parse(body, [](int depth, Value::parse_event_t, Value&) {
        if (depth > 16) throw std::invalid_argument("JSON nesting exceeds limit");
        return true;
    });
    if (!value.is_object()) throw std::invalid_argument("Expected a JSON object");
    return value;
}

}  // namespace json

#endif
