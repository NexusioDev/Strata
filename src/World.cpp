#include "World.hpp"
#include <cmath>

#include "TileRegistry.hpp"

World::World(int width, int height, unsigned int seed)
    : mWidth(width), mHeight(height), mSeed(seed), mNoise(seed) {
    
    mTiles.resize(width * height, Tile{TileType::Air});
    // VertexArray als Quads (4 Punkte pro Block) initialisieren
    mVertexArray.setPrimitiveType(sf::PrimitiveType::TriangleFan);
    if (!mTextureAtlas.loadFromFile("../assets/tileset.png")) {
        // Fehlerbehandlung
    }

    // Einstellungen für das Terrain
    float terrainScale = 0.05f;  // Höherer Wert = mehr Hügel
    float terrainAmp = 10.0f;    // Maximale Hügelhöhe

    // Einstellungen für Erze
    float oreScale = 0.12f;      // Größe der Erzadern
    float goldScale = 0.22f;
    float coalThreshold = 0.55f;  // Seltenheit (0.5 - 0.9)
    float ironThreshold = 0.65f;
    float goldThreshold = 0.75f;

    // World Gen
    for (int x = 0; x < mWidth; ++x) {
        // 1D Perlin Noise für die Oberfläche
        float noiseVal = mNoise.noise(x * terrainScale, 0.5);
        int surfaceY = static_cast<int>((mHeight / 3.0f) + (noiseVal * terrainAmp));

        for (int y = surfaceY; y < mHeight; ++y) {
            int index = y * mWidth + x;

            if (y == surfaceY) {
                mTiles[index].type = TileType::Grass;
            }
            else if (y < surfaceY + 6) {
                mTiles[index].type = TileType::Dirt;
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
                } else if (goldNoise > goldThreshold) {
                    mTiles[index].type = TileType::Gold;
                } else {
                    mTiles[index].type = TileType::Stone;
                }
            }
        }
    }

    isExposedToAir();
    updateGeometry();
}

bool World::isSolid(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return true; // Weltgrenzen sind solide
    TileType t = mTiles[y * mWidth + x].type;
    return TileRegistry::get().at(t).isSolid;
}

void World::setTile(int x, int y, TileType type) {
    if (x >= 0 && x < mWidth && y >= 0 && y < mHeight) {
        mTiles[y * mWidth + x].type = type;
        updateGeometry(); // WICHTIG: Grafik neu berechnen!
    }
}

TileType World::getTileType(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return TileType::Air;
    return mTiles[y * mWidth + x].type;
}

void World::isExposedToAir() {
    for (int x = 0; x < mWidth; ++x) {
        for (int y = 0; y < mHeight; ++y) {
            if (x + 1 >= mWidth || x - 1 <= 0) {
                /*if (getTileType(x - 1,y - 1) == TileType::Air) {
                    setTile(x, y, TileType::Grass);
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

int World::getTileVariation(int x, int y, TileType type) {
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

void World::updateGeometry() {
    mVertexArray.clear();
    mVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles); // Triangles statt Quads

    // 2 Dreiecke pro Block = 6 Vertices pro Block
    mVertexArray.resize(mWidth * mHeight * 6);

    int v = 0; // Vertex Index Counter
    float ts = mTileSize;
    float texSize = 16.f;

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            TileType type = mTiles[y * mWidth + x].type;
            if (type == TileType::Air) continue;

            // Infos aus Registry holen
            const auto& info = TileRegistry::get().at(type);
            int variation = getTileVariation(x, y, type);
            sf::Vector2f tex(info.textureCoords[variation].x * texSize, info.textureCoords[variation].y * texSize);

            sf::Vector2f pos(x * ts, y * ts);

            // --- Dreieck 1 ---
            mVertexArray[v + 0].position = {pos.x, pos.y};
            mVertexArray[v + 0].texCoords = {tex.x, tex.y}; // Oben Links (16px Basis)

            mVertexArray[v + 1].position = {pos.x + ts, pos.y};
            mVertexArray[v + 1].texCoords = {tex.x + texSize, tex.y}; // Oben Rechts (+16px)

            mVertexArray[v + 2].position = {pos.x, pos.y + ts};
            mVertexArray[v + 2].texCoords = {tex.x, tex.y + texSize}; // Unten Links (+16px)

            // --- Dreieck 2 ---
            mVertexArray[v + 3].position = {pos.x + ts, pos.y};
            mVertexArray[v + 3].texCoords = {tex.x + texSize, tex.y};

            mVertexArray[v + 4].position = {pos.x + ts, pos.y + ts};
            mVertexArray[v + 4].texCoords = {tex.x + texSize, tex.y + texSize};

            mVertexArray[v + 5].position = {pos.x, pos.y + ts};
            mVertexArray[v + 5].texCoords = {tex.x, tex.y + texSize};

            v += 6;
        }
    }
    mVertexArray.resize(v);
}

void World::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Wir zeichnen einfach das gesamte VertexArray auf einmal
    states.texture = &mTextureAtlas;
    target.draw(mVertexArray, states);
}