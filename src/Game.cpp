#include "Game.hpp"
#include <cmath> // Für std::floor

Game::Game() 
    : mWindow(sf::VideoMode({1280, 720}), "Strata - C++ Sandbox"), // {} für Vector2u
    mWorld(100, 50),
    mFpsTimer(0.f),
    mFrameCount(0),
    mFont(),
    mFpsText(mFont),
    mInventoryText(mFont) {

    if (!mFont.openFromFile("../assets/GoogleSans-Medium.ttf")) {
        // Fehlerbehandlung, falls die Datei fehlt
    }

    mFpsText.setCharacterSize(20);
    mFpsText.setFillColor(sf::Color::Yellow);
    mFpsText.setPosition({10.f, 10.f});

    mInventoryText.setCharacterSize(20);
    mInventoryText.setFillColor(sf::Color::White);
    mInventoryText.setPosition({10.f, 40.f}); // Y auf 40 schieben!

    mWorldView.setSize({1280.f, 720.f});
    mWorldView.setCenter({640.f, 360.f});

    mPlayer.setSize({40.f, 60.f});
    mPlayer.setFillColor(sf::Color::Red);
    mPlayer.setPosition({600.f, 300.f}); // Vector2f statt (x, y)
}

void Game::run() {
    sf::Clock clock;
    while (mWindow.isOpen()) {
        sf::Time deltaTime = clock.restart(); // Zeit seit dem letzten Frame messen
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
            int tileX = static_cast<int>(worldPos.x / 32.f);
            int tileY = static_cast<int>(worldPos.y / 32.f);

            TileType hitType = mWorld.getTileType(tileX, tileY);

            if (hitType == TileType::Dirt) {
                mInventoryDirt++;
            }
            if (hitType == TileType::Stone) { mInventoryStone++; }

            mWorld.setTile(tileX, tileY, TileType::Air);

            sf::Color pColor = (hitType == TileType::Stone) ? sf::Color(100, 100, 100) : sf::Color(139, 69, 19);
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

        // Rechtsklick zum Bauen (optional)
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
            // Nur bauen, wenn wir Steine im Inventar haben!
            if (mInventoryStone > 0) {
                sf::Vector2f worldPos = mWindow.mapPixelToCoords(sf::Mouse::getPosition(mWindow), mWorldView);
                int tileX = static_cast<int>(std::floor(worldPos.x / 32.f));
                int tileY = static_cast<int>(std::floor(worldPos.y / 32.f));

                sf::FloatRect newTileRect({tileX * 32.f, tileY * 32.f}, {32.f, 32.f});

                // Check: Ist an der Stelle schon ein Block? (Verhindert Ressourcen-Verschwendung)
                if (mWorld.getTileType(tileX, tileY) == TileType::Air) {
                    if (!newTileRect.findIntersection(mPlayer.getGlobalBounds())) {
                        mWorld.setTile(tileX, tileY, TileType::Stone);
                        mInventoryStone--;
                    }
                }
            }
        }
    }
}

void Game::checkCollision(bool xDirection) {
    mIsOnGround = false; // Reset, wir prüfen es gleich neu
    sf::FloatRect playerBounds = mPlayer.getGlobalBounds();

    // Wir prüfen nur die Tiles in der unmittelbaren Umgebung des Spielers
    int startX = static_cast<int>(playerBounds.position.x / 32.f);
    int endX   = static_cast<int>((playerBounds.position.x + playerBounds.size.x) / 32.f);
    int startY = static_cast<int>(playerBounds.position.y / 32.f);
    int endY   = static_cast<int>((playerBounds.position.y + playerBounds.size.y) / 32.f);

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            if (mWorld.isSolid(x, y)) {
                sf::FloatRect tileBounds({x * 32.f, y * 32.f}, {32.f, 32.f});

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
                    // Wichtig: Bounds nach der Korrektur aktualisieren, falls wir mehrere Blöcke berühren
                    playerBounds = mPlayer.getGlobalBounds();
                }
            }
        }
    }
}

void Game::update(float dt) {
    float moveSpeed = 400.f; // Pixel pro Sekunde
    mVelocity.x = 0.f; // Horizontaler Stopp, wenn keine Taste gedrückt wird

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

    // 5. Springen (nur wenn auf dem Boden)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && mIsOnGround) {
        mVelocity.y = -500.f; // Ein kräftiger Impuls nach oben
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
    mDayTime += dt * 0.01f; // Geschwindigkeit des Tageslaufs
    if (mDayTime > 1.0f) mDayTime = 0.0f;

    mNightFactor = std::sin(mDayTime * 3.14159f * 2.0f);

    if (mNightFactor > 0.2f) {
        // Tag: Hellblau
        mSkyColor = sf::Color(135, 206, 235);
    } else if (mNightFactor < -0.2f) {
        mSkyColor = sf::Color(15, 15, 45);
    } else {
        mSkyColor = sf::Color(100, 80, 120);
    }
}

void Game::render() {
    mWindow.clear(mSkyColor); // Nutze die berechnete Himmelsfarbe!
    mWorldView.setCenter({ std::round(mPlayer.getPosition().x), std::round(mPlayer.getPosition().y) });

    mWindow.setView(mWorldView);
    mWindow.draw(mWorld);

    for (const auto& p : mParticles) {
        sf::RectangleShape shape({4.f, 4.f});
        shape.setPosition(p.pos);
        shape.setFillColor(p.color);
        mWindow.draw(shape);
    }

    mWindow.draw(mPlayer);

    // --- LICHT / NACHT EFFEKT ---
    if (mNightFactor < 0.0f) { // Wenn es Nacht wird
        mWindow.setView(mWindow.getDefaultView());
        // Einfache Vignette (Dunkler Rand)
        sf::RectangleShape vignette(sf::Vector2f(mWindow.getSize()));
        vignette.setFillColor(sf::Color(0, 0, 0, 180)); // Dunkelheit
        mWindow.draw(vignette);
    }

    // --- UI ---
    mWindow.setView(mWindow.getDefaultView());
    mWindow.draw(mFpsText);

    // Inventar-Anzeige
    mInventoryText.setString("Dirt: " + std::to_string(mInventoryDirt) +
                             " | Stone: " + std::to_string(mInventoryStone));
    mWindow.draw(mInventoryText);

    mWindow.display();
}