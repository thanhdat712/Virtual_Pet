// Wrapper biên dịch SQLite cho MinGW.org cũ (GCC 6.3.0) thiếu GetTickCount64.
// Cung cấp shim trả về ULONGLONG đúng chuẩn rồi mới include sqlite3.c.
#include <windows.h>

static ULONGLONG WINAPI sqlite3Shim_GetTickCount64(void) {
    return (ULONGLONG)GetTickCount();
}

#define GetTickCount64 sqlite3Shim_GetTickCount64
#include "sqlite3.c"
#undef GetTickCount64
