cmake_minimum_required(VERSION 3.20)

project(Pet_Raising)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(Pet_Raising
    backend/main.cpp
    backend/src/Core/Game.cpp
    backend/src/Core/GameState.cpp
)

target_include_directories(Pet_Raising PRIVATE
    backend/include/Core
)
