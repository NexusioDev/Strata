#ifndef STRATA_WORLD_HPP
#define STRATA_WORLD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>

// Ein einfacher Block-Typ
enum class TileType : uint8_t {
    Air = 0,
    Dirt = 1,
    Stone = 2,
    Grass = 3,
    Torch = 4,
};

struct Tile {
    TileType type = TileType::Air;
};

class World : public sf::Drawable { // Ermöglicht window.draw(world)
public:
    World(int width, int height);
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
    
    // Das Herzstück für Performance und Lücken-Freiheit
    sf::VertexArray mVertexArray;
};

#endif