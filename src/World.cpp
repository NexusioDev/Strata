#include "World.hpp"
#include <queue>

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
    mLight.resize(width * height, sf::Color::White);

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
    //generateCaves();
}

void World::update(float skyBrightness) {
    if (!mIsCalculatingLight) {
        for (int cy = 0; cy < mChunksY; ++cy)
            for (int cx = 0; cx < mChunksX; ++cx)
                if (mChunks[cy * mChunksX + cx].dirty)
                    rebuildChunk(cx, cy);
    }

    if (mLightNeedsUpdate && !mIsCalculatingLight) {
        mLightNeedsUpdate = false;
        mIsCalculatingLight = true;

        // Starte Thread
        std::thread([this, skyBrightness]() {
            asyncLightCalc(skyBrightness);
        }).detach();
    }
}

void World::asyncLightCalc(float brightness) {
    // 1. Lokale Arbeitskopie der Lichtwerte (0-15)
    std::vector<int> lightValues(mWidth * mHeight, 0);
    std::queue<sf::Vector2i> lightQueue;

    // Grundhelligkeit des Himmels
    int skyLevel = static_cast<int>(std::max(2.0f, 15.0f * brightness));

    // 2. Initialisierung (Himmel & Lichtquellen)
    for (int x = 0; x < mWidth; ++x) {
        bool seesSky = true;
        for (int y = 0; y < mHeight; ++y) {
            int idx = y * mWidth + x;
            TileType type = mTiles[idx].type;

            if (seesSky && type == TileType::Air) {
                lightValues[idx] = skyLevel;
                lightQueue.emplace(x, y);
            } else if (!TileRegistry::get().at(type).isLightSource) {
                seesSky = false;
            }

            if (TileRegistry::get().at(type).isLightSource) {
                lightValues[idx] = 14;
                lightQueue.emplace(x, y);
            }
        }
    }

    // 3. BFS Licht-Ausbreitung (Das Herzstück)
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    while (!lightQueue.empty()) {
        sf::Vector2i curr = lightQueue.front();
        lightQueue.pop();

        int currIdx = curr.y * mWidth + curr.x;
        int currL = lightValues[currIdx];

        for (int i = 0; i < 4; ++i) {
            int nx = curr.x + dx[i], ny = curr.y + dy[i];
            if (nx >= 0 && nx < mWidth && ny >= 0 && ny < mHeight) {
                int nIdx = ny * mWidth + nx;

                // Dämpfung: Festkörper schlucken mehr Licht
                int loss = isSolid(nx, ny) ? 3 : 1;
                int nextL = currL - loss;

                if (nextL > lightValues[nIdx]) {
                    lightValues[nIdx] = nextL;
                    lightQueue.push({nx, ny});
                }
            }
        }
    }

    // 4. Ergebnisse in das Haupt-Licht-Array übertragen
    for (int i = 0; i < mWidth * mHeight; ++i) {
        unsigned char c = static_cast<unsigned char>(std::max(20, lightValues[i] * 17));
        sf::Color newColor(c, c, c, 255);

        if (mLight[i] != newColor) {
            mLight[i] = newColor;
            // Chunk als dirty markieren, damit er im Hauptthread neu gebaut wird
            getChunk(i % mWidth, i / mWidth).dirty = true;
        }
    }

    // Flag zurücksetzen: Thread ist fertig!
    mIsCalculatingLight = false;
}

void World::forceLightUpdate() {
    mLightNeedsUpdate = true;
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
        mLightNeedsUpdate = true;
    }
}

TileType World::getTileType(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return TileType::Air;
    return mTiles[y * mWidth + x].type;
}

Tile& World::getTileRef(int x, int y) {
    // Falls außerhalb, geben wir ein Dummy-Air-Tile zurück oder klemmen den Wert ein
    x = std::clamp(x, 0, mWidth - 1);
    y = std::clamp(y, 0, mHeight - 1);
    return mTiles[y * mWidth + x];
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

const Chunk& World::getChunk(int tileX, int tileY) const {
    int cx = std::clamp(tileX / Chunk::SIZE, 0, mChunksX - 1);
    int cy = std::clamp(tileY / Chunk::SIZE, 0, mChunksY - 1);
    return mChunks[cy * mChunksX + cx];
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

float World::getBiomeNoise(int x) {
    return mNoise.noiseNorm(x * 0.005f, 333.3f);
}

/*void World::generateCaves() {
    // 1. Initialisierung: Fülle alles unter der Oberfläche zufällig
    // (Nutze deinen bestehenden surfaceY-Wert als Grenze)
    for (int x = 0; x < mWidth; ++x) {
        for (int y = 100; y < mHeight; ++y) { // Angenommen ab Tiefe 100
            if (mTiles[y * mWidth + x].type == TileType::Stone) {
                // 45% Chance, dass es Luft wird für den CA-Start
                if ((rand() % 100) < 45) mTiles[y * mWidth + x].type = TileType::Air;
            }
        }
    }

    // 2. CA-Schritte (Glättung zu organischen Höhlen)
    for (int step = 0; step < 4; ++step) {
        std::vector<Tile> nextGen = mTiles;
        for (int y = 1; y < mHeight - 1; ++y) {
            for (int x = 1; x < mWidth - 1; ++x) {
                int wallCount = 0;
                // Nachbarn zählen
                for (int ny = -1; ny <= 1; ++ny) {
                    for (int nx = -1; nx <= 1; ++nx) {
                        if (mTiles[(y + ny) * mWidth + (x + nx)].type != TileType::Air)
                            wallCount++;
                    }
                }
                // Regel: 4-5 Methode
                if (wallCount > 4) nextGen[y * mWidth + x].type = TileType::Stone;
                else nextGen[y * mWidth + x].type = TileType::Air;
            }
        }
        mTiles = nextGen;
    }
}*/

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

/*void World::generateLightningMap(float skyBrightness) {
    std::vector<int> lightValues(mWidth * mHeight, 0);
    std::queue<sf::Vector2i> lightQueue;

    int skyLevel = static_cast<int>(std::max(2.0f, 15.0f * skyBrightness));

    for (int x = 0; x < mWidth; ++x) {
        bool seesSky = true;
        for (int y = 0; y < mHeight; ++y) {
            int index = x + mWidth * y;
            TileType type = mTiles[index].type;

            if (seesSky && type == TileType::Air) {
                lightValues[index] = skyLevel;
                lightQueue.push({x, y});
            } else {
                seesSky = false;
            }

            if (TileRegistry::get().at(type).isLightSource) {
                lightValues[index] = 14; // Fackeln sind fast so hell wie die Sonne
                lightQueue.push({x, y});
            }
        }
    }

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    while (!lightQueue.empty()) {
        sf::Vector2i curr = lightQueue.front();
        lightQueue.pop();

        int currIdx = curr.y * mWidth + curr.x;
        int currL = lightValues[currIdx];

        for (int i = 0; i < 4; ++i) {
            int nx = curr.x + dx[i], ny = curr.y + dy[i];
            if (nx >= 0 && nx < mWidth && ny >= 0 && ny < mHeight) {
                int nIdx = ny * mWidth + nx;
                int loss = isSolid(nx, ny) ? 3 : 1;
                int nextL = currL - loss;

                if (nextL > lightValues[nIdx]) {
                    lightValues[nIdx] = nextL;
                    lightQueue.push({nx, ny});
                }
            }
        }
    }

    for (int i = 0; i < 15; i++) {
        bool changed = false;
        for (int y = 0; y < mHeight; ++y) {
            for (int x = 0; x < mWidth; ++x) {
                int index = x + mWidth * y;
                int currentLight = lightValues[index];
                if (currentLight <= 1) continue;

                int nx[4] = {x,x,x-1,x+1};
                int ny[4] = {y-1,y+1,y,y};

                for (int n = 0; n < 4; ++n) {
                    int nextX = nx[n];
                    int nextY = ny[n];

                    if (nextX >= 0 && nextX < mWidth && nextY >= 0 && nextY < mHeight) {
                        int nIndex = nextY * mWidth + nextX;

                        int loss = isSolid(nextX, nextY) ? 3 : 1;
                        int newLight = currentLight - loss;

                        if (newLight > lightValues[nIndex]) {
                            lightValues[nIndex] = newLight;
                            changed = true;
                        }
                    }
                }
            }
        }
        if (!changed) break;
    }

    for (int i = 0; i < mWidth * mHeight; ++i) {
        unsigned char c = static_cast<unsigned char>(std::max(20, lightValues[i] * 17));
        sf::Color newColor(c, c, c, 255);

        if (mLight[i] != newColor) {
            mLight[i] = newColor;
            getChunk(i % mWidth, i / mWidth).dirty = true;
        }
    }
    /*for (int x = 0; x < mWidth; ++x) {
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

                    if (depth == 0) mLight[index] = sf::Color(210,210,210,255);
                    else if (depth == 1) mLight[index] = sf::Color(135,135,135,255);
                    else if (depth == 2) mLight[index] = sf::Color(80,80,80,255);
                    else mLight[index] = sf::Color::Black;
                } else { // z.B eine Cave
                    mLight[index] = sf::Color::Black;
                }
            }
        }
    }#1#
}*/

sf::Color World::getLightAt(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return sf::Color::Black;
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

            sf::Color lightColor = mLight[gy * mWidth + gx];

            const auto& info = TileRegistry::get().at(type);
            int variation    = getTileVariation(gx, gy, type);
            sf::Vector2f tex(info.textureCoords[variation].x * texSize,
                             info.textureCoords[variation].y * texSize);
            sf::Vector2f pos(gx * ts, gy * ts);

            // Dreieck 1
            chunk.vertexArray[v+0].position  = {pos.x, pos.y};
            chunk.vertexArray[v+0].texCoords = {tex.x, tex.y};
            chunk.vertexArray[v+0].color = lightColor;
            chunk.vertexArray[v+1].position  = {pos.x + ts, pos.y};
            chunk.vertexArray[v+1].texCoords = {tex.x+texSize, tex.y};
            chunk.vertexArray[v+1].color = lightColor;
            chunk.vertexArray[v+2].position  = {pos.x, pos.y + ts};
            chunk.vertexArray[v+2].texCoords = {tex.x, tex.y+texSize};
            chunk.vertexArray[v+2].color = lightColor;
            // Dreieck 2
            chunk.vertexArray[v+3].position  = {pos.x + ts, pos.y};
            chunk.vertexArray[v+3].texCoords = {tex.x+texSize, tex.y};
            chunk.vertexArray[v+3].color = lightColor;
            chunk.vertexArray[v+4].position  = {pos.x + ts, pos.y + ts};
            chunk.vertexArray[v+4].texCoords = {tex.x+texSize, tex.y+texSize};
            chunk.vertexArray[v+4].color = lightColor;
            chunk.vertexArray[v+5].position  = {pos.x, pos.y + ts};
            chunk.vertexArray[v+5].texCoords = {tex.x, tex.y+texSize};
            chunk.vertexArray[v+5].color = lightColor;

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