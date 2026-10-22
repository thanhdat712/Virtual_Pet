#include "sqlite3.h"
#include <cstdio>

int printRow(void*, int count, char** values, char**) {
    for (int column = 0; column < count; ++column) {
        if (column) std::putchar('|');
        std::fputs(values[column] ? values[column] : "", stdout);
    }
    std::putchar('\n');
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    sqlite3* database = nullptr;
    if (sqlite3_open(argv[1], &database) != SQLITE_OK) return 3;
    sqlite3_busy_timeout(database,3000);
    char* message = nullptr;
    int status = sqlite3_exec(database,argv[2],printRow,nullptr,&message);
    if (message) { std::fprintf(stderr,"%s\n",message); sqlite3_free(message); }
    sqlite3_close(database);
    return status == SQLITE_OK ? 0 : 1;
}