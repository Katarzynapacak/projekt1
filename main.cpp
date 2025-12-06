#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Menu.h"
#include "Game.h"
#include "GameState.h"

struct ScoreBoard
{
    std::vector<int> entries;

    bool load(const std::string& filename)
    {
        entries.clear();

        std::ifstream file(filename);
        if (!file.is_open())
            return false;

        int value = 0;
        while (file >> value)
        {
            entries.push_back(value);
        }

        return true;
    }

    bool save(const std::string& filename) const
    {
        std::ofstream file(filename);
        if (!file.is_open())
            return false;

        for (const auto& v : entries)
            file << v << "\n";

        return true;
    }

    void add(int value)
    {
        entries.push_back(value);
        const std::size_t MAX_RESULTS = 10;
        if (entries.size() > MAX_RESULTS)
            entries.erase(entries.begin(), entries.begin() + (entries.size() - MAX_RESULTS));
    }

    std::string formatForDisplay() const
    {
        std::ostringstream oss;
        if (entries.empty())
        {
            oss << "Brak zapisanych wynikow.";
        }
        else
        {
            oss << "Ostatnie wyniki:\n";
            for (std::size_t i = 0; i < entries.size(); ++i)
            {
                oss << (i + 1) << ". " << entries[i] << " zbitych bloczkow\n";
            }
        }

        oss << "\nESC - powrot do menu";
        return oss.str();
    }
};

enum class GameStateEnum
{
    Menu,
    Playing,
    Paused,
    Scores,
    Exiting
};

int main()
{
    const int SZEROKOSC = 800;
    const int WYSOKOSC = 600;
    const std::string SCORE_FILE = "wyniki.txt";

    sf::RenderWindow window(
        sf::VideoMode(SZEROKOSC, WYSOKOSC),
        "Arkanoid"
    );
    window.setFramerateLimit(60);

    Menu menu(window.getSize().x, window.getSize().y);
    Game game;
    GameState snapshot;
    ScoreBoard scoreBoard;
    scoreBoard.load(SCORE_FILE);

    GameStateEnum currentState = GameStateEnum::Menu;

    sf::Clock clock;

    while (window.isOpen())
    {
        sf::Time dt = clock.restart();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed)
            {
                // MENU 
                if (currentState == GameStateEnum::Menu)
                {
                    if (event.key.code == sf::Keyboard::Up)
                        menu.moveUp();
                    else if (event.key.code == sf::Keyboard::Down)
                        menu.moveDown();
                    else if (event.key.code == sf::Keyboard::Enter)
                    {
                        int selected = menu.getSelectedItemIndex();

                        if (selected == 0)      // Nowa gra
                        {
                            game.reset();
                            currentState = GameStateEnum::Playing;
                        }
                        else if (selected == 1) // Wczytaj gre
                        {
                            GameState loaded;
                            if (loaded.loadFromFile("zapis.txt"))
                            {
                                loaded.apply(
                                    game.getPaddle(),
                                    game.getBall(),
                                    game.getBricks()
                                );
                                std::cout << "Gra wczytana!\n";
                                currentState = GameStateEnum::Playing;
                            }
                            else
                            {
                                std::cout << "Brak pliku zapisu lub blad odczytu.\n";
                            }
                        }
                        else if (selected == 2) // Ostatnie wyniki
                        {
                            currentState = GameStateEnum::Scores;
                        }
                        else if (selected == 3) // Wyjscie
                        {
                            currentState = GameStateEnum::Exiting;
                            window.close();
                        }
                    }
                }
                // PLAYING
                else if (currentState == GameStateEnum::Playing)
                {
                    // Zapis gry na F5 i przejscie do pauzy
                    if (event.key.code == sf::Keyboard::F5)
                    {
                        snapshot.capture(
                            game.getPaddle(),
                            game.getBall(),
                            game.getBlocks()
                        );

                        if (snapshot.saveToFile("zapis.txt"))
                            std::cout << "Gra zapisana!\n";
                        else
                            std::cout << "Blad zapisu!\n";

                        currentState = GameStateEnum::Paused;
                    }

                    if (event.key.code == sf::Keyboard::Escape)
                    {
                        scoreBoard.add(game.getDestroyedBricks());
                        scoreBoard.save(SCORE_FILE);
                        currentState = GameStateEnum::Menu;
                    }
                }
                // PAUSED
                else if (currentState == GameStateEnum::Paused)
                {
                    if (event.key.code == sf::Keyboard::F5)
                    {
                        // powrot do gry z zapisanym stanem
                        currentState = GameStateEnum::Playing;
                    }
                    else if (event.key.code == sf::Keyboard::Escape)
                    {
                        // wyjscie do menu bez kasowania zapisu
                        currentState = GameStateEnum::Menu;
                    }
                }
                // SCORES
                else if (currentState == GameStateEnum::Scores)
                {
                    if (event.key.code == sf::Keyboard::Escape)
                        currentState = GameStateEnum::Menu;
                }
            }
        }

        // UPDATE
        if (currentState == GameStateEnum::Playing)
        { 
            game.update(dt);

            if (game.isBallOutOfBounds(static_cast<float>(window.getSize().y)))
            {
                scoreBoard.add(game.getDestroyedBricks());
                scoreBoard.save(SCORE_FILE);
                game.reset();
                currentState = GameStateEnum::Menu;
            }
        }

        // RENDER
        window.clear(sf::Color(40, 30, 20));

        if (currentState == GameStateEnum::Menu)
        {
            menu.draw(window);
        }
        else if (currentState == GameStateEnum::Playing ||
            currentState == GameStateEnum::Paused)
        {
            game.render(window);

            if (currentState == GameStateEnum::Paused)
            {
                sf::Font font;
                font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

                sf::Text pausedText;
                pausedText.setFont(font);
                pausedText.setString("PAUZA\nF5 - powrot do gry\nESC - menu");
                pausedText.setCharacterSize(24);
                pausedText.setFillColor(sf::Color::White);
                pausedText.setPosition(40.f, 40.f);

                window.draw(pausedText);
            }
        }
        else if (currentState == GameStateEnum::Scores)
        { 
            sf::Font font;
            font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

            sf::Text txt;
            txt.setFont(font);
            txt.setString(scoreBoard.formatForDisplay());
            txt.setCharacterSize(24);
            txt.setFillColor(sf::Color::White);
            txt.setPosition(40.f, 40.f);

            window.draw(txt);
        }

        window.display();
    }

    return 0;
}
