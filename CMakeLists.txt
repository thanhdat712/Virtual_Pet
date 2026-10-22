cmake_minimum_required(VERSION 3.20)

project(Pet_Raising)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)

FetchContent_Declare(
    sqlite_amalgamation
    URL https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip
    URL_HASH SHA256=0A9C8E34352DAD46006AA82836D5383BFF9DC6BC9CE5A60553C05AA30E6AD1F9
)
FetchContent_GetProperties(sqlite_amalgamation)
if(NOT sqlite_amalgamation_POPULATED)
    FetchContent_Populate(sqlite_amalgamation)
endif()

add_library(sqlite3 STATIC
    ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_DEFAULT_FOREIGN_KEYS=1)

FetchContent_Declare(
    argon2_source
    URL https://github.com/P-H-C/phc-winner-argon2/archive/refs/tags/20190702.zip
    URL_HASH SHA256=506A80B90AC3CA8407636F3B26B7FED87A55F2F1275CDE850CD6698903C4F008
)
FetchContent_GetProperties(argon2_source)
if(NOT argon2_source_POPULATED)
    FetchContent_Populate(argon2_source)
endif()

add_library(argon2 STATIC
    ${argon2_source_SOURCE_DIR}/src/argon2.c
    ${argon2_source_SOURCE_DIR}/src/core.c
    ${argon2_source_SOURCE_DIR}/src/encoding.c
    ${argon2_source_SOURCE_DIR}/src/ref.c
    ${argon2_source_SOURCE_DIR}/src/thread.c
    ${argon2_source_SOURCE_DIR}/src/blake2/blake2b.c
)
target_include_directories(argon2 PUBLIC ${argon2_source_SOURCE_DIR}/include)
target_include_directories(argon2 PRIVATE
    ${argon2_source_SOURCE_DIR}/src
    ${argon2_source_SOURCE_DIR}/src/blake2
)
find_package(Threads REQUIRED)
target_link_libraries(argon2 PUBLIC Threads::Threads)

set(AUTH_HTTP_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/auth_http_include")
file(MAKE_DIRECTORY "${AUTH_HTTP_INCLUDE_DIR}/nlohmann")
file(DOWNLOAD
    https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h
    "${AUTH_HTTP_INCLUDE_DIR}/httplib.h"
    EXPECTED_HASH SHA256=A0A0C13DC086663863DBE6730A19716F7D3744904B6205496E8301750814DBD5
    TLS_VERIFY ON
    STATUS HTTPLIB_DOWNLOAD_STATUS
)
list(GET HTTPLIB_DOWNLOAD_STATUS 0 HTTPLIB_DOWNLOAD_CODE)
if(NOT HTTPLIB_DOWNLOAD_CODE EQUAL 0)
    message(FATAL_ERROR "Could not download cpp-httplib: ${HTTPLIB_DOWNLOAD_STATUS}")
endif()

file(DOWNLOAD
    https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp
    "${AUTH_HTTP_INCLUDE_DIR}/nlohmann/json.hpp"
    EXPECTED_HASH SHA256=AAF127C04CB31C406E5B04A63F1AE89369FCCDE6D8FA7CDDA1ED4F32DFC5DE63
    TLS_VERIFY ON
    STATUS JSON_DOWNLOAD_STATUS
)
list(GET JSON_DOWNLOAD_STATUS 0 JSON_DOWNLOAD_CODE)
if(NOT JSON_DOWNLOAD_CODE EQUAL 0)
    message(FATAL_ERROR "Could not download nlohmann/json: ${JSON_DOWNLOAD_STATUS}")
endif()

enable_testing()

# Thư viện chứa logic game; main và các test đều dùng chung
add_library(pet_core STATIC
    backend/src/Core/Game.cpp
    backend/src/Core/GameState.cpp
    backend/src/Item/Item.cpp
    backend/src/Item/Food.cpp
    backend/src/Item/Decoration.cpp
    backend/src/Inventory/Inventory.cpp
    backend/src/User/User.cpp
    backend/src/User/PlayerGameData.cpp
    backend/src/Shop/Shop.cpp
    backend/src/Reward/Reward.cpp
    backend/src/Pet/Pet.cpp
)

# Mỗi module có thư mục include riêng (code hiện tại include theo kiểu "Game.h", "../User/User.h")
target_include_directories(pet_core PUBLIC
    backend/include/Core
    backend/include/Item
    backend/include/Inventory
    backend/include/User
    backend/include/Shop
    backend/include/Reward
    backend/include/Pet
)

add_library(auth_service STATIC
    backend/src/Auth/AuthService.cpp
)
target_include_directories(auth_service PUBLIC
    backend/include/Auth
    backend/include/User
    "${AUTH_HTTP_INCLUDE_DIR}"
)
target_link_libraries(auth_service PUBLIC sqlite3 argon2 pet_core)
if(WIN32)
    target_link_libraries(auth_service PUBLIC bcrypt)
endif()

add_executable(Pet_Raising backend/main.cpp)
target_link_libraries(Pet_Raising PRIVATE pet_core)

add_executable(Pet_Raising_Server backend/server_main.cpp)
target_link_libraries(Pet_Raising_Server PRIVATE auth_service)
target_compile_definitions(
    Pet_Raising_Server
    PRIVATE PET_FRONTEND_DIR="${CMAKE_CURRENT_SOURCE_DIR}/frontend"
)
if(WIN32)
    target_link_libraries(Pet_Raising_Server PRIVATE ws2_32)
endif()

# Test
add_executable(test_game_state tests/test_game_state.cpp)
target_link_libraries(test_game_state PRIVATE pet_core)
add_test(NAME game_state COMMAND test_game_state)

add_executable(test_user tests/test_user.cpp)
target_link_libraries(test_user PRIVATE pet_core)
add_test(NAME user_validation COMMAND test_user)

add_executable(test_auth_service tests/test_auth_service.cpp)
target_link_libraries(test_auth_service PRIVATE auth_service)
add_test(NAME auth_registration COMMAND test_auth_service)

add_executable(test_reward_shop tests/test_reward_shop.cpp)
target_link_libraries(test_reward_shop PRIVATE pet_core)
add_test(NAME reward_shop COMMAND test_reward_shop)
