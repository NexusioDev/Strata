cmake_minimum_required(VERSION 4.1)
project(Strata)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)

add_executable(Strata src/main.cpp
        src/Game.hpp
        src/Game.cpp
        src/world.hpp
        src/World.cpp
        src/PerlinNoise.cpp
        src/PerlinNoise.hpp
        src/TileRegistry.hpp)

target_link_libraries(Strata PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)
