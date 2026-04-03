#ifndef STRATA_WORLD_HPP
#define STRATA_WORLD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
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
    Sand = 8,
    Sandstone = 9,
    Snow = 10,
    Wood = 11,
    Leaves = 12,
};

enum class Biome { Plains, Desert, Forest, Mountains };

struct BiomeInfo {
    float terrainAmp;       // Hügelstärke
    float terrainScale;     // Hügelfrequenz
    TileType surfaceTile;   // Oberster Block
    TileType subSurfaceTile;// Unter der Oberfläche
    int subSurfaceDepth;    // Wie tief der Sub-Surface geht
};

struct Tile {
    TileType type = TileType::Air;
};

struct TileInfo {
    std::string name;
    std::vector<sf::Vector2i> textureCoords;
    sf::Color mindMapColor;
    bool isSolid;
    bool isLightSource;
    int tileVariants;
    // Hier kannst du später mehr hinzufügen:
    // float hardness; (für Abbaugeschwindigkeit)
    // int lightEmission;
};

struct Chunk {
    static constexpr int SIZE = 16;
    sf::VertexArray vertexArray;
    bool dirty = true;
};

class World : public sf::Drawable { // Ermöglicht window.draw(world)
public:
    World(int width, int height, unsigned int seed);
    void setTile(int x, int y, TileType type);
    void update(float skyBrightness);

    void forceLightUpdate();

    float getTileSize() const { return mTileSize; }
    TileType getTileType(int x, int y) const;

    Tile &getTileRef(int x, int y);

    bool isSolid(int x, int y) const;

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

    void isExposedToAir();

    int getTileVariation(int x, int y, TileType type) const;
    sf::Color getLightning(int x, int y);

    std::atomic<bool> mIsCalculatingLight{false};
    void asyncLightCalc(float brightness);

private:
    // Diese Funktion wird von sf::Drawable gefordert
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    int mWidth;
    int mHeight;
    const float mTileSize = 20.f;
    std::vector<Tile> mTiles;
    std::vector<sf::Color> mLight;
    PerlinNoise mNoise;
    unsigned int mSeed;

    // Das Herzstück für Performance und Lücken-Freiheit
    sf::VertexArray mVertexArray;
    sf::Texture mTextureAtlas;

    //Chunks
    std::vector<Chunk> mChunks;
    int mChunksX, mChunksY;
    bool mLightNeedsUpdate = true;

    Chunk& getChunk(int tileX, int tileY);
    const Chunk& getChunk(int tileX, int tileY) const;

    Biome getBiome(int x);
    float getBiomeBlend(int x);
    float getBiomeNoise(int x);

    void generateTrees();

    void generateLightningMap(float skyBrightness);

    void rebuildChunk(int chunkX, int chunkY);
};

#endif