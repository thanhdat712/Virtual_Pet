#ifndef SERVER_CRYPTO_H
#define SERVER_CRYPTO_H
#include <string>

namespace crypto {
std::string randomHex(unsigned long bytes);
std::string passwordHash(const std::string& password, const std::string& salt);
bool constantEqual(const std::string& left, const std::string& right);
}
#endif