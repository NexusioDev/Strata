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
        src/TileRegistry.hpp
        src/discord.hpp
        src/discord.cpp)

target_link_libraries(Strata PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)
target_include_directories(Strata PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord_social_sdk/include")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/lib/discord_social_sdk/lib/release/discord_partner_sdk.lib")

add_custom_command(TARGET Strata POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord_social_sdk/bin/discord_partner_sdk.dll"
        $<TARGET_FILE_DIR:Strata>
)
