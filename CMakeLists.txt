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
    backend/src/Pet/Cat.cpp
    backend/src/Pet/Dog.cpp
)

# Mỗi module có thư mục include riêng (code hiện tại include theo kiểu "Game.h", "../User/User.h")
target_include_directories(pet_core PUBLIC
    backend
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
)
target_link_libraries(auth_service PUBLIC sqlite3 argon2 pet_core)
if(WIN32)
    target_link_libraries(auth_service PUBLIC bcrypt)
endif()

add_executable(Pet_Raising backend/main.cpp)
target_link_libraries(Pet_Raising PRIVATE pet_core)

add_executable(Pet_Raising_Server
    backend/src/server_main.cpp
    backend/src/Exercise/Exercise.cpp
    backend/src/Exercise/Submission.cpp
    backend/src/Server/Database.cpp
    backend/src/Server/HttpServer.cpp
    backend/src/Server/Crypto.cpp
    backend/src/Server/Catalog.cpp
)
target_include_directories(Pet_Raising_Server PRIVATE
    backend/include/Server
    backend/include/Exercise
    backend/include/Pet
    backend/third_party
    backend/third_party/sqlite
)
target_link_libraries(Pet_Raising_Server PRIVATE sqlite3)
target_compile_definitions(
    Pet_Raising_Server
    PRIVATE PET_FRONTEND_DIR="${CMAKE_CURRENT_SOURCE_DIR}/frontend"
)
if(WIN32)
    target_link_libraries(Pet_Raising_Server PRIVATE ws2_32)
else()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(Pet_Raising_Server PRIVATE OpenSSL::Crypto)
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

add_executable(test_game_flow tests/test_game_flow.cpp)
target_link_libraries(test_game_flow PRIVATE pet_core)
add_test(NAME game_flow COMMAND test_game_flow)


# Database Access Layer (INT-02)
add_library(pet_db STATIC
    backend/src/Database/DatabaseConnection.cpp
    backend/src/Database/UserRepository.cpp
    backend/src/Database/PetRepository.cpp
    backend/src/Database/InventoryRepository.cpp
    backend/src/Database/GameRepository.cpp
)
target_include_directories(pet_db PUBLIC
    backend/include/Database
    backend/include/Core
    backend/include/User
    backend/include/Item
    backend/include/Inventory
    backend/include/Pet
    backend/include/Shop
    backend/include/Reward
)
target_link_libraries(pet_db PUBLIC sqlite3 pet_core)

add_executable(test_database tests/test_database.cpp)
target_link_libraries(test_database PRIVATE pet_db)
add_test(NAME database_access COMMAND test_database)

# PET-06: clamps, feeding, polymorphism and time decay
add_executable(test_pet_requirements tests/tests/test_pet_requirements.cpp)
target_link_libraries(test_pet_requirements PRIVATE pet_core)
add_test(NAME pet_requirements COMMAND test_pet_requirements)

# SUB-01/SUB-03: submission and test-case model validation
add_executable(test_submission_models
    tests/tests/test_submission_models.cpp
    backend/src/Exercise/Submission.cpp
    backend/src/Exercise/TestCase.cpp
)
target_include_directories(test_submission_models PRIVATE backend/include/Exercise)
add_test(NAME submission_models COMMAND test_submission_models)

add_executable(test_exercise_scoring
    tests/tests/test_exercise_scoring.cpp
    backend/src/Exercise/Exercise.cpp
)
target_include_directories(test_exercise_scoring PRIVATE backend/include/Exercise)
add_test(NAME exercise_scoring COMMAND test_exercise_scoring)
