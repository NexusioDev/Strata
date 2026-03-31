#include "Game.hpp"
#include "discordpp.h"
#include <cmath>
#include <thread>
#include <string>
#include <functional>
#include <iostream>

#include "TileRegistry.hpp"

Game::Game()
    : mWindow(sf::VideoMode({1280, 720}), "Strata - C++ Sandbox"), // {} für Vector2u
    mWorld(512, 512, static_cast<unsigned int>(std::time(nullptr))),
    mFpsTimer(0.f),
    mFrameCount(0),
    mFont(),
    mFpsText(mFont),
    mInventoryText(mFont),
    mLightSprite(mLightMap.getTexture()) {

    if (!mFont.openFromFile("../assets/GoogleSans-Medium.ttf")) {
        // Fehlerbehandlung, falls die Datei fehlt
    }

    if (!mLightMap.resize({1280, 720})) {
        // Fehlerbehandlung
    }

    mTorchLight.setRadius(200.f);
    mTorchLight.setOrigin({200.f, 200.f});
    mTorchLight.setFillColor(sf::Color(255, 255, 255, 255));

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

    mTorchLight.setRadius(120.f);
    mTorchLight.setOrigin({120.f, 120.f});
    mTorchLight.setFillColor(sf::Color(255, 255, 255, 220));
    // TODO: Make a nice gradient texture instead of flat circle
}

void Game::run() {
    sf::Clock clock;
    const auto client = std::make_shared<discordpp::Client>();
    client->SetApplicationId(1488543536788934706);
    discordpp::Activity activity;
    activity.SetName("Strata - C++ Sandbox");
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetDetails("Am Spielen bruh");
    activity.SetState("Am Entwickeln");

    client->UpdateRichPresence(activity, [](const discordpp::ClientResult &result) {
        if (result.Successful()) {
            std::cout << "Updatete";
        }
        else {
            std::cout << "Nicht update: " << result.Error();
        }
    });


    while (mWindow.isOpen()) {
        sf::Time deltaTime = clock.restart(); // Zeit seit dem letzten Frame messen
        if (client) {
            discordpp::RunCallbacks();
        }
        processEvents();
        float dt = deltaTime.asSeconds();
        if (dt > 0.05f) dt = 0.05f; // Maximal 20 FPS "Simulations-Zeit" pro Frame
        update(dt); // Zeit an update übergeben
        render();
    }
}

void Game::processEvents() {
    while (const std::optional event = mWindow.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            mWindow.close();
        }

        // Linksklick zum Abbauen
        if (event->is<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
            int tileX = static_cast<int>(worldPos.x / mWorld.getTileSize());
            int tileY = static_cast<int>(worldPos.y / mWorld.getTileSize());

            TileType hitType = mWorld.getTileType(tileX, tileY);

            if (hitType == TileType::Dirt || hitType == TileType::Grass) {
                mInventoryDirt++;
            }
            if (hitType == TileType::Stone) { mInventoryStone++; }

            mWorld.setTile(tileX, tileY, TileType::Air);

            if (hitType != TileType::Air) {
                sf::Color pColor = TileRegistry::get().at(hitType).mindMapColor;
                for (int i = 0; i < 8; ++i) { // 8 kleine Splitter
                    Particle p;
                    p.pos = worldPos;
                    // Zufällige Richtung und Geschwindigkeit
                    p.vel = { (static_cast<float>(rand() % 100) - 50.f) * 2.f,
                              (static_cast<float>(rand() % 100) - 80.f) * 2.f };
                    p.lifetime = 1.0f; // 1 Sekunde sichtbar
                    p.color = pColor; // Dreck-Braun
                    mParticles.push_back(p);
                }
            }
        }

        // Rechtsklick zum Bauen
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
            if (mInventoryStone > 0) {
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
                int tileX = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
                int tileY = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));

                sf::FloatRect newTileRect({tileX * mWorld.getTileSize(), tileY * mWorld.getTileSize()}, {mWorld.getTileSize(), mWorld.getTileSize()});

                // Check: Ist an der Stelle schon ein Block? (Verhindert Ressourcen-Verschwendung)
                if (mWorld.getTileType(tileX, tileY) == TileType::Air) {
                    if (!newTileRect.findIntersection(mPlayer.getGlobalBounds())) {
                        mWorld.setTile(tileX, tileY, TileType::Stone);
                        mInventoryStone--;
                    }
                }
            }
        }

        // TODO: Put in Right Mouse Click with Inventar System (Crafting?)
        if (event->is<sf::Event::KeyPressed>()) {
            auto* key = event->getIf<sf::Event::KeyPressed>();
            if (key && key->code == sf::Keyboard::Key::T) {
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
                int tx = static_cast<int>(std::floor(worldPos.x / mWorld.getTileSize()));
                int ty = static_cast<int>(std::floor(worldPos.y / mWorld.getTileSize()));

                if (tx >= 0 && tx < mWorld.getWidth() && ty >= 0 && ty < mWorld.getHeight() &&
                    mWorld.getTileType(tx, ty) == TileType::Air)
                {
                    // Fackel als Block setzen
                    mWorld.setTile(tx, ty, TileType::Torch);

                    // mInventoryTorch--;
                }
            }
        }

        //Debug, export world as Picture
        if (event->is<sf::Event::KeyPressed>()) {
            auto* key = event->getIf<sf::Event::KeyPressed>();
            if (key && key->code == sf::Keyboard::Key::M) {
                sf::Image image(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()));
                for (int x = 0; x < mWorld.getWidth(); ++x) {
                    for (int y = 0; y < mWorld.getHeight(); ++y) {
                        TileType tileType = mWorld.getTileType(x, y);
                        sf::Color tileColor = TileRegistry::get().at(tileType).mindMapColor;
                        image.setPixel(sf::Vector2u(x, y), tileColor);
                    }
                }
                if (image.saveToFile("mein_bild_sfml3.png")) {
                    // Erfolg!
                }

                sf::Image imageLight(sf::Vector2u(mWorld.getWidth(), mWorld.getHeight()));
                for (int x = 0; x < mWorld.getWidth(); ++x) {
                    for (int y = 0; y < mWorld.getHeight(); ++y) {
                        sf::Color lightColor = mWorld.getLightning(x,y);
                        imageLight.setPixel(sf::Vector2u(x, y), lightColor);
                    }
                }
                if (imageLight.saveToFile("lightning_sfml3.png")) {
                    // Erfolg!
                }
            }
        }
    }
}

void Game::checkCollision(bool xDirection) {
    mIsOnGround = false;
    sf::FloatRect playerBounds = mPlayer.getGlobalBounds();

    // Only Check Tiles in near of the Player (Performance)
    int startX = static_cast<int>(playerBounds.position.x / mWorld.getTileSize());
    int endX   = static_cast<int>((playerBounds.position.x + playerBounds.size.x) / mWorld.getTileSize());
    int startY = static_cast<int>(playerBounds.position.y / mWorld.getTileSize());
    int endY   = static_cast<int>((playerBounds.position.y + playerBounds.size.y) / mWorld.getTileSize());

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            if (mWorld.isSolid(x, y)) {
                sf::FloatRect tileBounds({x * mWorld.getTileSize(), y * mWorld.getTileSize()}, {mWorld.getTileSize(), mWorld.getTileSize()});

                // Schnittmenge berechnen
                if (auto intersection = playerBounds.findIntersection(tileBounds)) {
                    if (xDirection) {
                        // Horizontal korrigieren
                        if (mVelocity.x > 0)
                            mPlayer.setPosition({tileBounds.position.x - playerBounds.size.x, mPlayer.getPosition().y});
                        else
                            mPlayer.setPosition({tileBounds.position.x + tileBounds.size.x, mPlayer.getPosition().y});
                        mVelocity.x = 0;
                    } else {
                        // Vertikal korrigieren
                        if (mVelocity.y > 0) { // Fallen
                            mPlayer.setPosition({mPlayer.getPosition().x, tileBounds.position.y - playerBounds.size.y});
                            mVelocity.y = 0;
                            mIsOnGround = true;
                        } else if (mVelocity.y < 0) { // Kopfstoß
                            mPlayer.setPosition({mPlayer.getPosition().x, tileBounds.position.y + tileBounds.size.y});
                            mVelocity.y = 0;
                        }
                    }

                    playerBounds = mPlayer.getGlobalBounds();
                }
            }
        }
    }
}

void Game::update(float dt) {
    mWorld.update();
    float moveSpeed = 300.f; // Pixel pro Sekunde
    mVelocity.x = 0.f;

    // 1. Input
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        mVelocity.x = -moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        mVelocity.x = moveSpeed;

    // 2. Schwerkraft anwenden
    mVelocity.y += mGravity * dt;

    // 3. Bewegung anwenden
    // --- HORIZONTALE BEWEGUNG & KOLLISION ---
    mPlayer.move({mVelocity.x * dt, 0.f});
    checkCollision(true); // true = wir prüfen X

    // --- VERTIKALE BEWEGUNG & KOLLISION ---
    mPlayer.move({0.f, mVelocity.y * dt});
    checkCollision(false); // false = wir prüfen Y

    // 4. Springen
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && mIsOnGround) {
        mVelocity.y = -500.f;
    }

    mFpsTimer += dt;
    mFrameCount++;

    if (mFpsTimer >= 1.f) { // Jede Sekunde aktualisieren
        mFpsText.setString("FPS: " + std::to_string(mFrameCount));
        mFrameCount = 0;
        mFpsTimer = 0.f;
    }

    /*
     * Partikel
     */
    for (auto& p : mParticles) {
        p.vel.y += mGravity * 0.5f * dt;
        p.pos += p.vel * dt;
        p.lifetime -= dt;
    }
    // Abgelaufene Partikel löschen
    mParticles.erase(std::remove_if(mParticles.begin(), mParticles.end(),
               [](const Particle& p) { return p.lifetime <= 0; }), mParticles.end());

    /*
     * Tag-Nach Zyklus
     */
    mDayTime += dt * 0.01f; // Geschwindigkeit des Tageslaufs → Je 100 sek
    if (mDayTime > 1.0f) mDayTime = 0.0f;

    // === Tageshelligkeit für Lightmap (wird in render() benutzt) ===
    float t = std::fmod(mDayTime, 1.f);
    float sun = std::max(0.f, std::sin(t * 6.28318f + 1.57f));
    mBrightness = (sun + 1.f) * 0.5f;   // 0.0 = Mitternacht, 1.0 = Mittag

    sf::Color day(135, 206, 235);
    sf::Color night(15, 15, 45);

    mSkyColor = sf::Color(
        static_cast<uint8_t>(day.r * sun + night.r * (1-sun)),
        static_cast<uint8_t>(day.g * sun + night.g * (1-sun)),
        static_cast<uint8_t>(day.b * sun + night.b * (1-sun))
    );
}

void Game::render() {
    // --- Standard Camera Follow
    mWorldView.setCenter({ std::round(mPlayer.getPosition().x), std::round(mPlayer.getPosition().y) });
    mWindow.setView(mWorldView);

    mWindow.clear(mSkyColor);

    // Welt, Partikel + Spieler
    mWindow.draw(mWorld);
    for (const auto& p : mParticles) {
        sf::RectangleShape shape({4.f, 4.f});
        shape.setPosition(p.pos);
        shape.setFillColor(p.color);
        mWindow.draw(shape);
    }
    mWindow.draw(mPlayer);

    // --- Lightmaps
    mLightMap.clear(getAmbientColor());
    mLightMap.setView(mWorldView);

    // Nur Tiles im Sichtbereich prüfen (Performance)
    int viewTileLeft   = static_cast<int>((mPlayer.getPosition().x - 640) / mWorld.getTileSize()) - 5;
    int viewTileRight  = static_cast<int>((mPlayer.getPosition().x + 640) / mWorld.getTileSize()) + 5;
    int viewTileTop    = static_cast<int>((mPlayer.getPosition().y - 360) / mWorld.getTileSize()) - 5;
    int viewTileBottom = static_cast<int>((mPlayer.getPosition().y + 360) / mWorld.getTileSize()) + 5;

    for (int y = std::max(0, viewTileTop); y <= std::min(mWorld.getHeight() - 1, viewTileBottom); ++y) {
        for (int x = std::max(0, viewTileLeft); x <= std::min(mWorld.getWidth() - 1, viewTileRight); ++x) {
            TileType t = mWorld.getTileType(x, y);
            if (TileRegistry::get().at(t).isLightSource) {
                // Position = **Mitte des Tile** – Welt-Koordinaten!
                sf::Vector2f torchPos(x * mWorld.getTileSize() + 16.f, y * mWorld.getTileSize() + 16.f);

                mTorchLight.setPosition(torchPos);          // ← entscheidend: immer neu setzen!
                mTorchLight.setRadius(140.f);
                mTorchLight.setFillColor(sf::Color(255, 190, 100, 220));

                mLightMap.draw(mTorchLight, sf::BlendAdd);
            }
        }
    }

    mWindow.setView(mWindow.getDefaultView());
    mLightMap.display();

    // --- Licht auf die Szene projection
    mWindow.setView(mWindow.getDefaultView());
    mLightSprite.setTexture(mLightMap.getTexture(), true);
    mWindow.draw(mLightSprite, sf::BlendMultiply);

    // UI (unbeleuchtet)
    mWindow.draw(mFpsText);
    mInventoryText.setString("Dirt: " + std::to_string(mInventoryDirt) +
                             " | Stone: " + std::to_string(mInventoryStone));
    mWindow.draw(mInventoryText);

    mWindow.display();
}

sf::Color Game::getAmbientColor() const {
    // brightness aus update() – am besten als Member-Variable speichern!
    float ambient = 0.18f + 0.82f * mBrightness;  // 18% nachts → ~100% mittags
    uint8_t val = static_cast<uint8_t>(ambient * 255);
    return sf::Color(val, val, val, 255);        // Grau → neutral
    // oder leicht getönt: sf::Color(val*0.95, val, val*1.05, 255);
}