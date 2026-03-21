#include "World.hpp"
#include <cmath>

#include "TileRegistry.hpp"

World::World(int width, int height, unsigned int seed)
    : mWidth(width), mHeight(height), mSeed(seed), mNoise(seed) {
    
    mTiles.resize(width * height, Tile{TileType::Air});
    // VertexArray als Quads (4 Punkte pro Block) initialisieren
    mVertexArray.setPrimitiveType(sf::PrimitiveType::TriangleFan);

    // Einstellungen für das Terrain
    float terrainScale = 0.05f;  // Höherer Wert = mehr Hügel
    float terrainAmp = 10.0f;    // Maximale Hügelhöhe

    // Einstellungen für Erze
    float oreScale = 0.12f;      // Größe der Erzadern
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
                mTiles[index].color = TileRegistry::get().at(TileType::Grass).color;
            }
            else if (y < surfaceY + 6) {
                mTiles[index].type = TileType::Dirt;
                mTiles[index].color = TileRegistry::get().at(TileType::Dirt).color;
            }
            else {
                // 2D Noise, für Ores etc.
                double coalNoise = mNoise.noise(x * oreScale, y * oreScale);
                double ironNoise = mNoise.noise(x * oreScale + 100 * 2, y * oreScale * 2);
                double goldNoise = mNoise.noise(x * oreScale + 200 * 3, y * oreScale * 3);

                if (coalNoise > coalThreshold) {
                    mTiles[index].type = TileType::Coal; // Oder ein anderes Erz
                    mTiles[index].color = TileRegistry::get().at(TileType::Coal).color;
                } else if (ironNoise > ironThreshold) {
                    mTiles[index].type = TileType::Iron;
                    mTiles[index].color = TileRegistry::get().at(TileType::Iron).color;
                } else if (goldNoise > goldThreshold) {
                    mTiles[index].type = TileType::Gold;
                    mTiles[index].color = TileRegistry::get().at(TileType::Gold).color;
                } else {
                    mTiles[index].type = TileType::Stone;
                    mTiles[index].color = TileRegistry::get().at(TileType::Stone).color;
                }
            }
        }
    }

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
        mTiles[y * mWidth + x].color = TileRegistry::get().at(type).color;
        updateGeometry(); // WICHTIG: Grafik neu berechnen!
    }
}

TileType World::getTileType(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return TileType::Air;
    return mTiles[y * mWidth + x].type;
}

void World::updateGeometry() {
    mVertexArray.clear();
    mVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles); // Triangles statt Quads

    // 2 Dreiecke pro Block = 6 Vertices pro Block
    mVertexArray.resize(mWidth * mHeight * 6);

    int v = 0; // Vertex Index Counter
    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            TileType type = mTiles[y * mWidth + x].type;
            if (type == TileType::Air) continue;

            sf::Color color = mTiles[y * mWidth + x].color;
            sf::Vector2f pos(x * mTileSize, y * mTileSize);

            // Dreieck 1
            mVertexArray[v + 0] = { {pos.x, pos.y}, color };                         // Oben Links
            mVertexArray[v + 1] = { {pos.x + mTileSize, pos.y}, color };             // Oben Rechts
            mVertexArray[v + 2] = { {pos.x, pos.y + mTileSize}, color };             // Unten Links

            // Dreieck 2
            mVertexArray[v + 3] = { {pos.x + mTileSize, pos.y}, color };             // Oben Rechts
            mVertexArray[v + 4] = { {pos.x + mTileSize, pos.y + mTileSize}, color }; // Unten Rechts
            mVertexArray[v + 5] = { {pos.x, pos.y + mTileSize}, color };             // Unten Links

            v += 6;
        }
    }
    mVertexArray.resize(v);
}

void World::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Wir zeichnen einfach das gesamte VertexArray auf einmal
    target.draw(mVertexArray, states);
}