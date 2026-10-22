#ifndef SERVER_CATALOG_H
#define SERVER_CATALOG_H
#include "Json.h"

const json::Value& shopCatalog();
const json::Value* shopItem(int id);
#endif