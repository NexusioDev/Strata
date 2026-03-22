#ifndef STRATA_TILE_REGISTRY_HPP
#define STRATA_TILE_REGISTRY_HPP

#include <map>
#include "World.hpp"

class TileRegistry {
public:
     // TileType Info Verknüpfungen etc.
    static const std::map<TileType, TileInfo>& get() {
        static const std::map<TileType, TileInfo> data = {
            { TileType::Air,   {"Luft", {{0,0}}, sf::Color::Transparent, false} },
            { TileType::Dirt,  {"Erde", {{1,0}}, sf::Color(139, 69, 19), true} },
            { TileType::Stone, {"Stein", {{2,0}}, sf::Color(100, 100, 100), true} },
            { TileType::Grass, {"Gras", {{0,1},{1,1}, {2,1},
            {0,2}, {2,2}, {0,3},{2,3}}, sf::Color(34, 139, 34), true, false, 7} },
            { TileType::Coal,  {"Kohle", {{3,0}}, sf::Color(30, 30, 30), true} },
            { TileType::Iron,  {"Eisen", {{5,0}}, sf::Color(155, 155, 155), true} },
            { TileType::Gold,  {"Gold", {{4,0}}, sf::Color(255, 215, 0), true} },
            { TileType::Torch,  {"Fackel", {{6,0}}, sf::Color(235, 144, 0), false, true} }
        };
        return data;
    }
};

#endif //STRATA_TILE_REGISTRY_HPP