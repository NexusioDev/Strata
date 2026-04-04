#ifndef STRATA_GAME_HPP
#define STRATA_GAME_HPP

#include <SFML/Graphics.hpp>
#include "World.hpp"

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float lifetime = 0.f;
    sf::Color color;
};

class Game {
public:
    Game();
    void run();

private:
    void processEvents();
    void update(float dt);
    void checkCollision(bool xDirection);
    void render();
    sf::Color getAmbientColor() const;

    sf::RenderWindow mWindow;
    sf::RectangleShape mPlayer;
    sf::View mWorldView;

    sf::Vector2f mVelocity{0.f, 0.f};
    bool mIsOnGround = false;
    const float mGravity = 9.81f * 100.f; // 9.81 m/s^2 skaliert auf Pixel

    std::vector<Particle> mParticles;
    World mWorld;

    sf::Font mFont;
    sf::Text mFpsText;
    sf::Text mInventoryText;
    float mFpsTimer = 0.f;
    int mFrameCount = 0;

    float mDayTime = 0.5f;
    float mBrightness = 1.f;
    float mLastLightUpdateBrightness = -1.f;
    sf::Color mSkyColor = sf::Color::Black;

    int mInventoryDirt = 0;
    int mInventoryStone = 0;

    // Licht
    sf::Image mLightImage;
    sf::Texture mLightTexture;
    sf::Sprite mLightSprite;
    bool mLightTextureInitialized = false;
};

#endif