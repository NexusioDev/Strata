#ifndef STRATA_GAME_HPP
#define STRATA_GAME_HPP

#include <SFML/Graphics.hpp>
#include "World.hpp"

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float lifetime;
    sf::Color color;
};

class Game {
public:
    Game();         // Konstruktor (Initialisierung)
    void run();     // Die Hauptschleife

private:
    void processEvents(); // Tastatur/Maus
    void update(float dt);        // Logik (Physik, Bewegung)
    void checkCollision(bool xDirection); //Kollisionen
    void render();        // Zeichnen

    sf::RenderWindow mWindow;
    sf::RectangleShape mPlayer; // Unser Platzhalter für den Spieler
    sf::View mWorldView; // Das hat gefehlt!
    float mPlayerSpeed = 200.f;  // Pixel pro Sekunde
    sf::Vector2f mVelocity{0.f, 0.f};
    bool mIsOnGround = false;
    const float mGravity = 9.81f * 100.f; // 9.81 m/s^2 skaliert auf Pixel
    const float mTileSize = 32.f;
    std::vector<Particle> mParticles;

    World mWorld;

    sf::Font mFont;
    sf::Text mFpsText;
    float mFpsTimer = 0.f;
    int mFrameCount = 0;

    float mDayTime = 0.5f; // 0.0 = Mitternacht, 0.5 = Mittag, 1.0 = Mitternacht
    sf::Color mSkyColor;
    float mNightFactor = 0.f;
    int mInventoryDirt = 0;
    int mInventoryStone = 0;
    sf::Text mInventoryText; // Zum Anzeigen der Items
};

#endif