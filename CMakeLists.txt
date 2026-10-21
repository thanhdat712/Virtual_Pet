cmake_minimum_required(VERSION 3.20)

project(Pet_Raising)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_executable(Pet_Raising backend/main.cpp)
target_link_libraries(Pet_Raising PRIVATE pet_core)

# Test
add_executable(test_game_state tests/test_game_state.cpp)
target_link_libraries(test_game_state PRIVATE pet_core)
add_test(NAME game_state COMMAND test_game_state)

add_executable(test_reward_shop backend/src/test_reward_shop.cpp)
target_link_libraries(test_reward_shop PRIVATE pet_core)
add_test(NAME reward_shop COMMAND test_reward_shop)
