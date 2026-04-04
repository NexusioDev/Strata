#include "Game.hpp"
#include "discord.hpp"
#include <cmath>
#include <string>
#include <algorithm>
#include "TileRegistry.hpp"

Game::Game()
    : mWindow(sf::VideoMode({1280, 720}), "Strata - C++ Sandbox"),
      mWorld(512, 512, static_cast<unsigned int>(std::time(nullptr))),
      mFpsTimer(0.f),
      mFrameCount(0),
      mFont(),
      mFpsText(mFont),
      mInventoryText(mFont),
      mLightSprite(mLightTexture)
{
    if (!mFont.openFromFile("../assets/GoogleSans-Medium.ttf")) {}

    mFpsText.setCharacterSize(20);
    mFpsText.setFillColor(sf::Color::Yellow);
    mFpsText.setPosition({10.f, 10.f});

    mInventoryText.setCharacterSize(20);
    mInventoryText.setFillColor(sf::Color::White);
    mInventoryText.setPosition({10.f, 40.f});

    mWorldView.setSize({1280.f, 720.f});
    mWorldView.setCenter({640.f, 360.f});

    mPlayer.setSize({38.f, 55.f});
    mPlayer.setFillColor(sf::Color::Red);
    mPlayer.setPosition({600.f, 300.f});
}

void Game::run() {
    sf::Clock clock;
    const sf::Time timeStep = sf::seconds(1.f / 60.f);
    sf::Time accumulator = sf::Time::Zero;

    DC_Instance dcInstance;
    dcInstance.InitDiscord();
    const auto client = dcInstance.discordClient;

    while (mWindow.isOpen()) {
        accumulator += clock.restart();

        if (client)
            discordpp::RunCallbacks();

        processEvents();

        while (accumulator >= timeStep) {
            update(timeStep.asSeconds());
            accumulator -= timeStep;
        }
        render();
    }
}

void Game::processEvents() {
    while (const std::optional event = mWindow.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            mWindow.close();

        // Linksklick — Abbauen
        if (event->is<sf::Event::MouseButtonPressed>()) {
            auto* mb = event->getIf<sf::Event::MouseButtonPressed>();
            if (mb && mb->button == sf::Mouse::Button::Left) {
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
                int tileX = static_cast<int>(worldPos.x / mWorld.getTileSize());
                int tileY = static_cast<int>(worldPos.y / mWorld.getTileSize());

                TileType hitType = mWorld.getTileType(tileX, tileY);
                if (hitType == TileType::Dirt || hitType == TileType::Grass) mInventoryDirt++;
                if (hitType == TileType::Stone) mInventoryStone++;

                mWorld.setTile(tileX, tileY, TileType::Air);

                if (hitType != TileType::Air) {
                    sf::Color pColor = TileRegistry::get().at(hitType).mindMapColor;
                    for (int i = 0; i < 8; ++i) {
                        Particle p;
                        p.pos = worldPos;
                        p.vel = { (static_cast<float>(rand() % 100) - 50.f) * 2.f, (static_cast<float>(rand() % 100) - 80.f) * 2.f };
                        p.lifetime = 1.0f;
                        p.color = pColor;
                        mParticles.push_back(p);
                    }
                }
            }
        }

        if (event->is<sf::Event::KeyPressed>()) {
            auto* key = event->getIf<sf::Event::KeyPressed>();
            if (!key) continue;

            // Fackel setzen
            if (key->code == sf::Keyboard::Key::T) {
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
                int tx = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
                int ty = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));
                if (tx >= 0 && tx < mWorld.getWidth() &&
                    ty >= 0 && ty < mWorld.getHeight() &&
                    mWorld.getTileType(tx, ty) == TileType::Air)
                {
                    mWorld.setTile(tx, ty, TileType::Torch);
                }
            }

            // Minimap + Lichtmap exportieren
            if (key->code == sf::Keyboard::Key::M) {
                sf::Image image(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()));
                for (int x = 0; x < mWorld.getWidth(); ++x)
                    for (int y = 0; y < mWorld.getHeight(); ++y)
                        image.setPixel({(unsigned)x, (unsigned)y},
                            TileRegistry::get().at(mWorld.getTileType(x, y)).mindMapColor);
                image.saveToFile("mein_bild_sfml3.png");

                sf::Image imageLight(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()));
                for (int x = 0; x < mWorld.getWidth(); ++x)
                    for (int y = 0; y < mWorld.getHeight(); ++y)
                        imageLight.setPixel({(unsigned)x, (unsigned)y},
                            mWorld.getLightAt(x, y));
                imageLight.saveToFile("lightning_sfml3.png");
            }
        }
    }

    // Rechtsklick — Bauen (kontinuierlich)
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) && mInventoryStone > 0) {
        sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
        int tileX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
        int tileY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));
        sf::FloatRect newTileRect(
            {tileX * mWorld.getTileSize(), tileY * mWorld.getTileSize()},
            {mWorld.getTileSize(), mWorld.getTileSize()});
        if (mWorld.getTileType(tileX, tileY) == TileType::Air &&
            !newTileRect.findIntersection(mPlayer.getGlobalBounds()))
        {
            mWorld.setTile(tileX, tileY, TileType::Stone);
            mInventoryStone--;
        }
    }
}

void Game::checkCollision(bool xDirection) {
    mIsOnGround = false;
    sf::FloatRect playerBounds = mPlayer.getGlobalBounds();

    int startX = static_cast<int>(playerBounds.position.x / mWorld.getTileSize());
    int endX = static_cast<int>((playerBounds.position.x + playerBounds.size.x) / mWorld.getTileSize());
    int startY = static_cast<int>(playerBounds.position.y / mWorld.getTileSize());
    int endY = static_cast<int>((playerBounds.position.y + playerBounds.size.y) / mWorld.getTileSize());

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            if (!mWorld.isSolid(x, y)) continue;
            sf::FloatRect tileBounds(
                {x * mWorld.getTileSize(), y * mWorld.getTileSize()},
                {mWorld.getTileSize(), mWorld.getTileSize()});
            if (!playerBounds.findIntersection(tileBounds)) continue;

            if (xDirection) {
                mPlayer.setPosition({mVelocity.x > 0 ? tileBounds.position.x - playerBounds.size.x : tileBounds.position.x + tileBounds.size.x, mPlayer.getPosition().y});
                mVelocity.x = 0;
            } else {
                if (mVelocity.y > 0) {
                    mPlayer.setPosition({mPlayer.getPosition().x,tileBounds.position.y - playerBounds.size.y});
                    mVelocity.y  = 0;
                    mIsOnGround  = true;
                } else if (mVelocity.y < 0) {
                    mPlayer.setPosition({mPlayer.getPosition().x,tileBounds.position.y + tileBounds.size.y});
                    mVelocity.y = 0;
                }
            }
            playerBounds = mPlayer.getGlobalBounds();
        }
    }
}

void Game::update(float dt) {
    mVelocity.x = 0.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) mVelocity.x = -300.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) mVelocity.x =  300.f;

    mVelocity.y += mGravity * dt;

    mPlayer.move({mVelocity.x * dt, 0.f});
    checkCollision(true);
    mPlayer.move({0.f, mVelocity.y * dt});
    checkCollision(false);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && mIsOnGround)
        mVelocity.y = -500.f;

    // FPS
    mFpsTimer += dt;
    mFrameCount++;
    if (mFpsTimer >= 1.f) {
        mFpsText.setString("FPS: " + std::to_string(mFrameCount));
        mFrameCount = 0;
        mFpsTimer = 0.f;
    }

    // Partikel
    for (auto& p : mParticles) {
        p.vel.y += mGravity * 0.5f * dt;
        p.pos += p.vel * dt;
        p.lifetime -= dt;
    }
    mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(),
        [](const Particle& p){ return p.lifetime <= 0.f; }), mParticles.end());

    // Tag/Nacht
    mDayTime += dt * 0.01f;
    if (mDayTime > 1.f) mDayTime = 0.f;
    float sun = std::max(0.f, std::sin(mDayTime * 6.28318f + 1.57f));
    mBrightness = (sun + 1.f) * 0.5f;

    sf::Color day(135, 206, 235), night(15, 15, 45);
    mSkyColor = sf::Color(
        static_cast<uint8_t>(day.r * sun + night.r * (1 - sun)),
        static_cast<uint8_t>(day.g * sun + night.g * (1 - sun)),
        static_cast<uint8_t>(day.b * sun + night.b * (1 - sun)));

    if (std::abs(mBrightness - mLastLightUpdateBrightness) > 0.05f) {
        mWorld.forceLightUpdate();
        mLastLightUpdateBrightness = mBrightness;
    }
    mWorld.update(mBrightness);
}

void Game::render() {
    // Licht-Textur einmalig initialisieren
    if (!mLightTextureInitialized) {
        mLightImage = sf::Image(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()),sf::Color::Black);
        mLightTexture.resize(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()));
        mLightTexture.setSmooth(true);
        mLightSprite.setTexture(mLightTexture, true);
        mLightTextureInitialized = true;
    }

    // Lichtpixel befüllen und Textur updaten
    for (int y = 0; y < mWorld.getHeight(); ++y)
        for (int x = 0; x < mWorld.getWidth(); ++x)
            mLightImage.setPixel({(unsigned)x, (unsigned)y}, mWorld.getLightAt(x, y));
    mLightTexture.update(mLightImage);

    mLightSprite.setScale({mWorld.getTileSize(), mWorld.getTileSize()});
    mLightSprite.setPosition({0.f, 0.f});

    // Kamera
    mWorldView.setCenter({
        std::round(mPlayer.getPosition().x),
        std::round(mPlayer.getPosition().y)});
    mWindow.setView(mWorldView);
    mWindow.clear(mSkyColor);

    // 1. Welt
    mWindow.draw(mWorld);

    // 2. Licht-Overlay
    //mWindow.draw(mLightSprite, sf::BlendMultiply);

    // 3. Partikel + Spieler
    for (const auto& p : mParticles) {
        sf::RectangleShape shape({4.f, 4.f});
        shape.setPosition(p.pos);
        shape.setFillColor(p.color);
        mWindow.draw(shape);
    }
    mWindow.draw(mPlayer);

    // 4. Tag/Nacht Overlay (Screen-Space)
    mWindow.setView(mWindow.getDefaultView());
    sf::RectangleShape darkOverlay({1280.f, 720.f});
    darkOverlay.setFillColor(sf::Color(0, 0, 0,
        static_cast<uint8_t>((1.f - mBrightness) * 180.f)));
    mWindow.draw(darkOverlay);

    // 5. UI
    mInventoryText.setString("Dirt: " + std::to_string(mInventoryDirt) +
                             " | Stone: " + std::to_string(mInventoryStone));
    mWindow.draw(mFpsText);
    mWindow.draw(mInventoryText);

    mWindow.display();
}

sf::Color Game::getAmbientColor() const {
    float   ambient = 0.18f + 0.82f * mBrightness;
    uint8_t val     = static_cast<uint8_t>(ambient * 255);
    return sf::Color(val, val, val, 255);
}