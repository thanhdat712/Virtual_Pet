#include "Crypto.h"
#include <windows.h>
#include <stdexcept>
#include <vector>

namespace {
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

std::string hex(const unsigned char* bytes, size_t count) {
    const char* digits = "0123456789abcdef";
    std::string result;
    for (size_t index = 0; index < count; ++index) {
        result += digits[bytes[index] >> 4];
        result += digits[bytes[index] & 15];
    }
    return result;
}
}

std::string crypto::randomHex(unsigned long bytes) {
    std::vector<unsigned char> buffer(bytes);
    if (cng().random(nullptr, buffer.data(), bytes, 2) < 0) throw std::runtime_error("CNG random failed");
    return hex(buffer.data(), buffer.size());
}

std::string crypto::passwordHash(const std::string& password, const std::string& salt) {
    Algorithm algorithm;
    if (cng().open(&algorithm.handle, L"SHA256", nullptr, 8) < 0) throw std::runtime_error("CNG SHA256 unavailable");
    DerivedKey key;
    LONG status = cng().derive(algorithm.handle,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()),
        reinterpret_cast<PUCHAR>(const_cast<char*>(salt.data())), static_cast<ULONG>(salt.size()),
        600000, key.bytes, sizeof(key.bytes), 0);
    if (status < 0) throw std::runtime_error("PBKDF2 failed");
    return "pbkdf2-sha256$600000$" + hex(key.bytes, sizeof(key.bytes));
}

bool crypto::constantEqual(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) return false;
    volatile unsigned char difference = 0;
    for (size_t index = 0; index < left.size(); ++index) difference |= left[index] ^ right[index];
    return difference == 0;
}