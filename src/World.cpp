#include "World.hpp"
#include <cmath>

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
    float oreThreshold = 0.55f;  // Seltenheit (0.5 - 0.9)

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
            else if (y < surfaceY + 4) {
                mTiles[index].type = TileType::Dirt;
            }
            else {
                // 2D Noise, für Ores etc.
                double oreNoise = mNoise.noise(x * oreScale, y * oreScale);

                if (oreNoise > oreThreshold) {
                    mTiles[index].type = TileType::Coal; // Oder ein anderes Erz
                } else {
                    mTiles[index].type = TileType::Stone;
                }
            }
        }
    }

    updateGeometry();
}

bool World::isSolid(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return true; // Weltgrenzen sind solide
    TileType t = mTiles[y * mWidth + x].type;
    return t != TileType::Air && t != TileType::Torch;
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

            sf::Vector2f pos(x * mTileSize, y * mTileSize);
            sf::Color color;
            if (type == TileType::Dirt) color = sf::Color(139, 69, 19);
            else if (type == TileType::Stone) color = sf::Color(100, 100, 100); // Grau für Stein
            else if (type == TileType::Grass) color = sf::Color(40 + rand()%40, 140 + rand()%60, 40 + rand()%30);
            else if (type == TileType::Torch) color = sf::Color(255, 180, 60);  // warm orange-gelb
            else color = sf::Color::White;

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