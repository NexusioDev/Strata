#ifndef STRATA_WORLD_HPP
#define STRATA_WORLD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include "PerlinNoise.hpp"

// Ein einfacher Block-Typ
enum class TileType : uint8_t {
    Air = 0,
    Dirt = 1,
    Stone = 2,
    Grass = 3,
    Torch = 4,
    Coal = 5,
    Iron = 6,
    Gold = 7,
};

struct Tile {
    TileType type = TileType::Air;
    sf::Color color = sf::Color::White;
};

struct TileInfo {
    std::string name;
    sf::Color color;
    bool isSolid;
    // Hier kannst du später mehr hinzufügen:
    // float hardness; (für Abbaugeschwindigkeit)
    // int lightEmission;
};

class World : public sf::Drawable { // Ermöglicht window.draw(world)
public:
    World(int width, int height, unsigned int seed);
    void setTile(int x, int y, TileType type);
    float getTileSize() const { return mTileSize; }
    TileType getTileType(int x, int y) const;

    bool isSolid(int x, int y) const;

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    
    // WICHTIG: Aktualisiert das VertexArray (muss gerufen werden, wenn sich Blöcke ändern)
    void updateGeometry();

private:
    // Diese Funktion wird von sf::Drawable gefordert
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    int mWidth;
    int mHeight;
    const float mTileSize = 20.f;
    std::vector<Tile> mTiles;
    PerlinNoise mNoise;
    unsigned int mSeed;
    
    // Das Herzstück für Performance und Lücken-Freiheit
    sf::VertexArray mVertexArray;
};

#endif