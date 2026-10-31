#include "Crypto.h"
#include <climits>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

#include <stdexcept>

namespace {
#ifdef _WIN32
using OpenProvider = LONG (WINAPI*)(void**, LPCWSTR, LPCWSTR, ULONG);
using CloseProvider = LONG (WINAPI*)(void*, ULONG);
using DeriveKey = LONG (WINAPI*)(void*, PUCHAR, ULONG, PUCHAR, ULONG, ULONGLONG, PUCHAR, ULONG, ULONG);
using GenerateRandom = LONG (WINAPI*)(void*, PUCHAR, ULONG, ULONG);

struct Cng {
    HMODULE library = LoadLibraryW(L"bcrypt.dll");
    OpenProvider open = library ? reinterpret_cast<OpenProvider>(GetProcAddress(library, "BCryptOpenAlgorithmProvider")) : nullptr;
    CloseProvider close = library ? reinterpret_cast<CloseProvider>(GetProcAddress(library, "BCryptCloseAlgorithmProvider")) : nullptr;
    DeriveKey derive = library ? reinterpret_cast<DeriveKey>(GetProcAddress(library, "BCryptDeriveKeyPBKDF2")) : nullptr;
    GenerateRandom random = library ? reinterpret_cast<GenerateRandom>(GetProcAddress(library, "BCryptGenRandom")) : nullptr;
    Cng() {
        if (!open || !close || !derive || !random) {
            if (library) FreeLibrary(library);
            throw std::runtime_error("Windows CNG unavailable");
        }
    }
    ~Cng() { if (library) FreeLibrary(library); }
};

Cng& cng() { static Cng instance; return instance; }

struct Algorithm {
    void* handle = nullptr;
    ~Algorithm() { if (handle) cng().close(handle, 0); }
};

struct DerivedKey {
    unsigned char bytes[32] = {};
    ~DerivedKey() {
        volatile unsigned char* wipe = bytes;
        for (size_t index = 0; index < sizeof(bytes); ++index) wipe[index] = 0;
    }
};
#else
struct DerivedKey {
    unsigned char bytes[32] = {};
    ~DerivedKey() { OPENSSL_cleanse(bytes, sizeof(bytes)); }
};
#endif

std::string hex(const unsigned char* bytes, size_t count) {
    const char* digits = "0123456789abcdef";
    std::string result;
    for (size_t index = 0; index < count; ++index) {
        result += digits[bytes[index] >> 4];
        result += digits[bytes[index] & 15];
    }
    return result;
}

std::string hexBytes(const std::vector<unsigned char>& bytes) {
    return hex(bytes.data(), bytes.size());
}
}

std::string crypto::randomHex(unsigned long bytes) {
    if (bytes > static_cast<unsigned long>(INT_MAX))
        throw std::runtime_error("Random byte request is too large");
    std::vector<unsigned char> buffer(bytes);
#ifdef _WIN32
    if (cng().random(nullptr, buffer.data(), bytes, 2) < 0) throw std::runtime_error("CNG random failed");
#else
    if (RAND_bytes(buffer.data(), static_cast<int>(buffer.size())) != 1)
        throw std::runtime_error("OpenSSL random generation failed");
#endif
    return hexBytes(buffer);
}

std::string crypto::passwordHash(const std::string& password, const std::string& salt) {
    DerivedKey key;
#ifdef _WIN32
    Algorithm algorithm;
    if (cng().open(&algorithm.handle, L"SHA256", nullptr, 8) < 0) throw std::runtime_error("CNG SHA256 unavailable");
    LONG status = cng().derive(algorithm.handle,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()),
        reinterpret_cast<PUCHAR>(const_cast<char*>(salt.data())), static_cast<ULONG>(salt.size()),
        600000, key.bytes, sizeof(key.bytes), 0);
    if (status < 0) throw std::runtime_error("PBKDF2 failed");
#else
    if (password.size() > static_cast<size_t>(INT_MAX) || salt.size() > static_cast<size_t>(INT_MAX) ||
        PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
            reinterpret_cast<const unsigned char*>(salt.data()), static_cast<int>(salt.size()),
            600000, EVP_sha256(), sizeof(key.bytes), key.bytes) != 1)
        throw std::runtime_error("PBKDF2 failed");
#endif
    return "pbkdf2-sha256$600000$" + hex(key.bytes, sizeof(key.bytes));
}

bool crypto::constantEqual(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) return false;
    volatile unsigned char difference = 0;
    for (size_t index = 0; index < left.size(); ++index) difference |= left[index] ^ right[index];
    return difference == 0;
}