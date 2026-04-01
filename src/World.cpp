#include "World.hpp"
#include <cmath>

#include "TileRegistry.hpp"

static const std::unordered_map<Biome, BiomeInfo> sBiomeTable = {
    { Biome::Plains,    { 10.f, 0.05f, TileType::Grass, TileType::Dirt,  6 }},
    { Biome::Desert,    {  5.f, 0.04f, TileType::Sand,  TileType::Sandstone, 10 }},
    { Biome::Forest,    { 14.f, 0.06f, TileType::Grass, TileType::Dirt,  5 }},
    { Biome::Mountains, { 35.f, 0.03f, TileType::Snow,  TileType::Stone, 2 }},
};

World::World(int width, int height, unsigned int seed)
    : mWidth(width), mHeight(height), mSeed(seed), mNoise(seed) {
    
    mTiles.resize(width * height, Tile{TileType::Air});
    mLight.resize(width * height, sf::Color::Magenta);

    mChunksX = (width + Chunk::SIZE - 1) / Chunk::SIZE;
    mChunksY = (height + Chunk::SIZE - 1) / Chunk::SIZE;
    mChunks.resize(mChunksX * mChunksY);

    if (!mTextureAtlas.loadFromFile("../assets/tileset.png")) {
        // Fehlerbehandlung
    }

    // Einstellungen für Erze
    float oreScale = 0.12f;      // Größe der Erzadern
    float goldScale = 0.22f;
    float coalThreshold = 0.55f;  // Seltenheit (0.5 - 0.9)
    float ironThreshold = 0.65f;
    float goldThreshold = 0.75f;
    int goldHeight = mHeight / 2;

    // Caves
    float caveScale = 0.15f;
    float caveThreshold = 0.70f;
    int deepCaveHeight = mHeight / 3 * 2;
    float deepCaveScale = 0.05f;
    float deepCaveThreshold = 0.40f;

    // World Gen
    for (int x = 0; x < mWidth; ++x) {
        float noiseVal = getBiomeNoise(x);
        float noiseFwd = getBiomeNoise(x + 48); // 48 Tiles voraus

        Biome biomeA = getBiome(x);
        Biome biomeB = getBiome(x + 48);
        const BiomeInfo& infoA = sBiomeTable.at(biomeA);
        const BiomeInfo& infoB = sBiomeTable.at(biomeB);

        float diff  = std::abs(noiseVal - noiseFwd);
        float t     = std::clamp(diff * 5.f, 0.f, 1.f);
        float blend = t * t * (3.f - 2.f * t);

        float noiseA = mNoise.noiseNorm(x * infoA.terrainScale, 0.5f);
        float noiseB = mNoise.noiseNorm(x * infoB.terrainScale, 0.5f);
        int surfaceA = static_cast<int>((mHeight / 3.0f) + noiseA * infoA.terrainAmp);
        int surfaceB = static_cast<int>((mHeight / 3.0f) + noiseB * infoB.terrainAmp);
        int surfaceY = static_cast<int>(surfaceA * (1.f - blend) + surfaceB * blend);

        TileType surfaceTile;
        TileType subTile;
        int subDepth;

        surfaceTile = infoA.surfaceTile;
        subTile     = infoA.subSurfaceTile;
        subDepth    = infoA.subSurfaceDepth;

        for (int y = surfaceY; y < mHeight; ++y) {
            int index = y * mWidth + x;

            if (y == surfaceY) {
                mTiles[index].type = surfaceTile;
            }
            else if (y < surfaceY + subDepth) {
                mTiles[index].type = subTile;
            }
            else {
                // 2D Noise, für Ores etc.
                double coalNoise = mNoise.noise(x * oreScale, y * oreScale);
                double ironNoise = mNoise.noise(x * oreScale + 100 * 2, y * oreScale * 2);
                double goldNoise = mNoise.noise(x * goldScale + 200, y * goldScale);

                if (coalNoise > coalThreshold) {
                    mTiles[index].type = TileType::Coal; // Oder ein anderes Erz
                } else if (ironNoise > ironThreshold) {
                    mTiles[index].type = TileType::Iron;
                } else if (goldNoise > goldThreshold && y >= goldHeight) {
                    mTiles[index].type = TileType::Gold;
                } else {
                    mTiles[index].type = TileType::Stone;
                }

                // Caves
                double caveNoise = mNoise.noise(x * caveScale + 1000, y * caveScale + 1000);
                double deepCaveNoise = mNoise.noise(x * deepCaveScale + 2000, y * deepCaveScale + 2000);
                if (y < deepCaveHeight) {
                    if (caveNoise > caveThreshold) {
                        mTiles[index].type = TileType::Air;
                    }
                } else if (y >= deepCaveHeight) {
                    if (deepCaveNoise > deepCaveThreshold) {
                        mTiles[index].type = TileType::Air;
                    }
                }
            }
        }
    }

    isExposedToAir();
    generateTrees();
    generateLightningMap();
}

void World::update() {
    for (int cy = 0; cy < mChunksY; ++cy)
        for (int cx = 0; cx < mChunksX; ++cx)
            if (mChunks[cy * mChunksX + cx].dirty)
                rebuildChunk(cx, cy);
}

bool World::isSolid(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return true; // Weltgrenzen sind solide
    TileType t = mTiles[y * mWidth + x].type;
    return TileRegistry::get().at(t).isSolid;
}

void World::setTile(int x, int y, TileType type) {
    if (x >= 0 && x < mWidth && y >= 0 && y < mHeight) {
        mTiles[y * mWidth + x].type = type;
        getChunk(x, y).dirty = true;
    }
}

TileType World::getTileType(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return TileType::Air;
    return mTiles[y * mWidth + x].type;
}

void World::isExposedToAir() {
    for (int x = 0; x < mWidth; ++x) {
        for (int y = 0; y < mHeight; ++y) {
            if (x == 0 || x == mWidth - 1) {
                /*if (getTileType(x - 1,y - 1) == TileType::Air) {
                    setTile(x, y, TileType::Grass);
                x + 1 >= mWidth || x - 1 <= 0
                }*/
            }
            else if (getTileType(x,y) == TileType::Dirt) {
                bool north = getTileType(x,y - 1) == TileType::Air;
                bool northEast = getTileType(x + 1,y - 1) == TileType::Air;
                bool northWest = getTileType(x - 1,y - 1) == TileType::Air;
                bool east = getTileType(x + 1,y) == TileType::Air;
                bool west = getTileType(x - 1,y) == TileType::Air;
                if (north|| northEast || northWest || east || west) {
                    setTile(x, y, TileType::Grass);
                }
            }
        }
    }
}

Chunk& World::getChunk(int tileX, int tileY) {
    int cx = tileX / Chunk::SIZE;
    int cy = tileY / Chunk::SIZE;
    // Clampen, damit wir nie out-of-bounds gehen
    cx = std::clamp(cx, 0, mChunksX - 1);
    cy = std::clamp(cy, 0, mChunksY - 1);
    return mChunks[cy * mChunksX + cx];
}

int World::getTileVariation(int x, int y, TileType type) const {
    if (type == TileType::Grass) {
        bool up = getTileType(x,y - 1) == TileType::Grass;
        bool down = getTileType(x,y + 1) == TileType::Grass;
        bool right = getTileType(x + 1,y) == TileType::Grass;
        bool left = getTileType(x - 1,y) == TileType::Grass;
        if (down && right) return 0;
        if (left && right) return 1;
        if (left && down) return 2;
        if (down && up && getTileType(x - 1,y) == TileType::Air) return 3;
        if (down && up && getTileType(x + 1,y) == TileType::Air) return 4;
        if (left && up) return 5;
        if (up && right) return 6;
        else return 1;
    }
    else {
        return 0;
    }
}

Biome World::getBiome(int x) {
    float val = getBiomeNoise(x);

    if (val < -0.4f) return Biome::Desert;    // 30%
    if (val <  0.0f) return Biome::Plains;    // 25%
    if (val <  0.4f) return Biome::Forest;    // 25%
    return Biome::Mountains;                   // 20%
}

float World::getBiomeBlend(int x) {
    float val = getBiomeNoise(x);
    float prev = getBiomeNoise(x - 48);
    float diff = std::abs(val - prev);
    float t = std::clamp(diff * 4.f, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float World::getBiomeNoise(int x) {
    return mNoise.noiseNorm(x * 0.005f, 333.3f);
}

void World::generateTrees() {
    for (int x = 2; x < mWidth - 2; ++x) {
        Biome biome = getBiome(x);
        if (biome == Biome::Desert || biome == Biome::Mountains) continue;

        float treeNoise = mNoise.noiseNorm(x * 0.3f, 777.7f);
        float threshold = (biome == Biome::Forest) ? 0.5f : 0.75f;
        if (treeNoise < threshold) continue;

        // Oberfläche finden
        int surfaceY = -1;
        for (int y = 0; y < mHeight; ++y) {
            if (mTiles[y * mWidth + x].type == TileType::Grass) {
                surfaceY = y;
                break;
            }
        }
        if (surfaceY < 0) continue;

        // Mindestabstand zum letzten Baum
        bool tooClose = false;
        for (int dx = -3; dx <= 3; ++dx) {
            int nx = x + dx;
            if (nx < 0 || nx >= mWidth) continue;
            if (mTiles[(surfaceY - 1) * mWidth + nx].type == TileType::Wood) {
                tooClose = true;
                break;
            }
        }
        if (tooClose) continue;

        // Baumhöhe variiert per Noise
        int trunkHeight = 4 + static_cast<int>(mNoise.noiseNorm(x * 0.5f, 888.8f) * 2.f + 2.f);

        // Stamm setzen
        for (int i = 1; i <= trunkHeight; ++i)
            setTile(x, surfaceY - i, TileType::Wood);

        // Blätter
        int topY = surfaceY - trunkHeight;
        for (int ly = -2; ly <= 1; ++ly) {
            int radius = (ly == -2 || ly == 1) ? 1 : 2;
            for (int lx = -radius; lx <= radius; ++lx) {
                int tx = x + lx;
                int ty = topY + ly;
                if (tx < 0 || tx >= mWidth || ty < 0) continue;
                if (mTiles[ty * mWidth + tx].type == TileType::Air)
                    setTile(tx, ty, TileType::Leaves);
            }
        }
    }
}

void World::generateLightningMap() {
    for (int x = 0; x < mWidth; ++x) {
        bool sunLight = true;
        int firstSolidY = -1;

        for (int y = 0; y < mHeight; ++y) {
            int index = y * mWidth + x;
            TileType currentType = getTileType(x, y);
            bool isSolid = TileRegistry::get().at(currentType).isSolid;

            if (sunLight && !isSolid) {
                mLight[index] = sf::Color::White;
            }
            else {
                if (sunLight) {
                    sunLight = false;
                    firstSolidY = y;
                }

                if (isSolid) {
                    int depth = y - firstSolidY;

                    if (depth == 0) mLight[index] = sf::Color(210,210,210,1);
                    else if (depth == 1) mLight[index] = sf::Color(135,135,135,1);
                    else if (depth == 2) mLight[index] = sf::Color(80,80,80,1);
                    else mLight[index] = sf::Color::Black;
                } else { // z.B eine Cave
                    mLight[index] = sf::Color::Black;
                }
            }
        }
    }
}

sf::Color World::getLightning(int x, int y) {
    return mLight[y * mWidth + x];
}

void World::rebuildChunk(int chunkX, int chunkY) {
    Chunk& chunk = mChunks[chunkY * mChunksX + chunkX];
    chunk.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
    chunk.vertexArray.resize(Chunk::SIZE * Chunk::SIZE * 6);

    int v = 0;
    float ts = mTileSize;
    float texSize = 16.f;

    int tileX0 = chunkX * Chunk::SIZE;
    int tileY0 = chunkY * Chunk::SIZE;

    for (int ly = 0; ly < Chunk::SIZE; ++ly) {
        for (int lx = 0; lx < Chunk::SIZE; ++lx) {
            int gx = tileX0 + lx;   // globale Tile-Koordinate
            int gy = tileY0 + ly;
            if (gx >= mWidth || gy >= mHeight) continue;

            TileType type = mTiles[gy * mWidth + gx].type;
            if (type == TileType::Air) continue;

            const auto& info = TileRegistry::get().at(type);
            int variation    = getTileVariation(gx, gy, type);
            sf::Vector2f tex(info.textureCoords[variation].x * texSize,
                             info.textureCoords[variation].y * texSize);
            sf::Vector2f pos(gx * ts, gy * ts);

            // Dreieck 1
            chunk.vertexArray[v+0].position  = {pos.x, pos.y};
            chunk.vertexArray[v+0].texCoords = {tex.x, tex.y};
            chunk.vertexArray[v+1].position  = {pos.x + ts, pos.y};
            chunk.vertexArray[v+1].texCoords = {tex.x+texSize, tex.y};
            chunk.vertexArray[v+2].position  = {pos.x, pos.y + ts};
            chunk.vertexArray[v+2].texCoords = {tex.x, tex.y+texSize};
            // Dreieck 2
            chunk.vertexArray[v+3].position  = {pos.x + ts, pos.y};
            chunk.vertexArray[v+3].texCoords = {tex.x+texSize, tex.y};
            chunk.vertexArray[v+4].position  = {pos.x + ts, pos.y + ts};
            chunk.vertexArray[v+4].texCoords = {tex.x+texSize, tex.y+texSize};
            chunk.vertexArray[v+5].position  = {pos.x, pos.y + ts};
            chunk.vertexArray[v+5].texCoords = {tex.x, tex.y+texSize};

            v += 6;
        }
    }
    chunk.vertexArray.resize(v);
    chunk.dirty = false;
}

void World::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    sf::Vector2f center = target.getView().getCenter();
    sf::Vector2f size   = target.getView().getSize();

    // Tile-Koordinaten des Viewports
    int cx0 = static_cast<int>((center.x - size.x * 0.5f) / mTileSize / Chunk::SIZE) - 1;
    int cy0 = static_cast<int>((center.y - size.y * 0.5f) / mTileSize / Chunk::SIZE) - 1;
    int cx1 = static_cast<int>((center.x + size.x * 0.5f) / mTileSize / Chunk::SIZE) + 1;
    int cy1 = static_cast<int>((center.y + size.y * 0.5f) / mTileSize / Chunk::SIZE) + 1;

    // Clampen
    cx0 = std::clamp(cx0, 0, mChunksX - 1);
    cy0 = std::clamp(cy0, 0, mChunksY - 1);
    cx1 = std::clamp(cx1, 0, mChunksX - 1);
    cy1 = std::clamp(cy1, 0, mChunksY - 1);

    states.texture = &mTextureAtlas;
    for (int cy = cy0; cy <= cy1; ++cy)
        for (int cx = cx0; cx <= cx1; ++cx)
            target.draw(mChunks[cy * mChunksX + cx].vertexArray, states);
}