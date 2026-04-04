#ifndef STRATA_WORLD_HPP
#define STRATA_WORLD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdint>
#include <atomic>
#include <thread>
#include "PerlinNoise.hpp"

enum class TileType : uint8_t {
    Air = 0, Dirt = 1, Stone = 2, Grass = 3,
    Torch = 4, Coal = 5, Iron = 6, Gold = 7,
    Sand = 8, Sandstone = 9, Snow = 10,
    Wood = 11, Leaves = 12,
};

enum class Biome { Plains, Desert, Forest, Mountains };

struct BiomeInfo {
    float terrainAmp;
    float terrainScale;
    TileType surfaceTile;
    TileType subSurfaceTile;
    int subSurfaceDepth;
};

struct Tile {
    TileType type = TileType::Air;
};

struct TileInfo {
    std::string name;
    std::vector<sf::Vector2i> textureCoords;
    sf::Color mindMapColor;
    bool isSolid = false;
    bool isLightSource = false;
    int tileVariants = 1;
};

struct Chunk {
    static constexpr int SIZE = 16;
    sf::VertexArray vertexArray;
    bool dirty = true;
};

class World : public sf::Drawable {
public:
    World(int width, int height, unsigned int seed);

    void setTile(int x, int y, TileType type);
    void update(float skyBrightness);
    void forceLightUpdate();

    float getTileSize() const { return mTileSize; }
    TileType getTileType(int x, int y) const;
    bool isSolid(int x, int y) const;
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    sf::Color getLightAt(int x, int y)  const;
    Tile& getTileRef(int x, int y);

    void isExposedToAir();
    int  getTileVariation(int x, int y, TileType type) const;

    std::atomic<bool> mIsCalculatingLight{false};

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    int mWidth, mHeight;
    const float mTileSize = 20.f;

    std::vector<Tile> mTiles;
    std::vector<sf::Color> mLight;

    PerlinNoise mNoise;
    unsigned int mSeed;
    sf::Texture mTextureAtlas;

    std::vector<Chunk> mChunks;
    int mChunksX = 0, mChunksY = 0;

    bool mLightNeedsUpdate = true;

    Chunk& getChunk(int tileX, int tileY);
    const Chunk& getChunk(int tileX, int tileY) const;
    void rebuildChunk(int chunkX, int chunkY);

    Biome getBiome(int x);
    float getBiomeNoise(int x);

    void generateTrees();
    void asyncLightCalc(float brightness);
};

#endif