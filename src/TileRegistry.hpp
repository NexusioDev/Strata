#ifndef STRATA_TILE_REGISTRY_HPP
#define STRATA_TILE_REGISTRY_HPP

#include <map>
#include "World.hpp"

class TileRegistry {
public:
     // TileType Info Verknüpfungen etc.
    static const std::map<TileType, TileInfo>& get() {
        static const std::map<TileType, TileInfo> data = {
            { TileType::Air,   {"Luft",   sf::Color::Transparent, false} },
            { TileType::Dirt,  {"Erde",   sf::Color(139, 69, 19), true} },
            { TileType::Stone, {"Stein",  sf::Color(100, 100, 100), true} },
            { TileType::Grass, {"Gras",   sf::Color(50, 150, 50), true} },
            { TileType::Coal,  {"Kohle",  sf::Color(30, 30, 30), true} },
            { TileType::Iron,  {"Eisen",  sf::Color(180, 150, 130), true} },
            { TileType::Gold,  {"Gold",   sf::Color(255, 215, 0), true} },
            { TileType::Torch,  {"Fackel",   sf::Color(255, 180, 60), false} }
        };
        return data;
    }
};

#endif //STRATA_TILE_REGISTRY_HPP