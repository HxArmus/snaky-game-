// main.cpp - minimal Snake with SFML (C++17)
// Compile (linux/mac): g++ main.cpp -o snake -std=c++17 -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <deque>
#include <random>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>

const int BLOCK = 20;
const int COLS = 40;                 // grid width
const int ROWS = 30;                 // grid height
const int WIDTH = COLS * BLOCK;
const int HEIGHT = ROWS * BLOCK;

struct Snake {
    std::deque<sf::Vector2i> body;
    sf::Vector2i dir{1,0};
    bool grow = false;
    Snake(sf::Vector2i start) { body.push_back(start); }
    sf::Vector2i nextHead() const { return body.front() + dir; }
    void move() {
        body.push_front(nextHead());
        if (!grow) body.pop_back();
        else grow = false;
    }
    bool hitsSelf() const {
        for (size_t i = 1; i < body.size(); ++i)
            if (body[i] == body.front()) return true;
        return false;
    }
    bool occupies(const sf::Vector2i& p) const {
        for (auto &b : body) if (b == p) return true;
        return false;
    }
};

static std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

sf::Vector2i spawnFood(const Snake &snake) {
    if ((int)snake.body.size() >= COLS * ROWS) return {-1,-1}; // full board
    std::uniform_int_distribution<int> dx(0, COLS-1), dy(0, ROWS-1);
    sf::Vector2i p;
    do { p = {dx(rng), dy(rng)}; } while (snake.occupies(p));
    return p;
}

int loadHighscore(const std::string &path="highscore.txt") {
    std::ifstream in(path);
    int h = 0; if (in >> h) return h; return 0;
}
void saveHighscore(int s, const std::string &path="highscore.txt") {
    std::ofstream out(path);
    if (out) out << s;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Snake (SFML)");
    window.setFramerateLimit(60);

    // Basic shapes
    sf::RectangleShape blockShape(sf::Vector2f(BLOCK-1, BLOCK-1));
    blockShape.setOutlineThickness(0);

    // Font (place a ttf in working dir)
    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/arial.ttf") || font.loadFromFile("arial.ttf");

    sf::Text scoreText, infoText;
    if (fontLoaded) {
        scoreText.setFont(font); scoreText.setCharacterSize(18); scoreText.setPosition(6,6);
        infoText.setFont(font); infoText.setCharacterSize(28); infoText.setPosition(8, HEIGHT/2 - 30);
    }

    // Init game state
    auto resetGame = [&]() {
        Snake s({COLS/2, ROWS/2});
        return s;
    };
    Snake snake = resetGame();
    sf::Vector2i food = spawnFood(snake);
    float moveDelay = 0.12f; // seconds between moves
    float acc = 0.f;
    sf::Clock clock;
    bool gameOver = false;
    int score = 0;
    int highscore = loadHighscore();

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::KeyPressed) {
                if (!gameOver) {
                    if (ev.key.code == sf::Keyboard::Up)    { if (snake.dir != sf::Vector2i(0,1))  snake.dir = {0,-1}; }
                    if (ev.key.code == sf::Keyboard::Down)  { if (snake.dir != sf::Vector2i(0,-1)) snake.dir = {0,1}; }
                    if (ev.key.code == sf::Keyboard::Left)  { if (snake.dir != sf::Vector2i(1,0))  snake.dir = {-1,0}; }
                    if (ev.key.code == sf::Keyboard::Right) { if (snake.dir != sf::Vector2i(-1,0)) snake.dir = {1,0}; }
                    if (ev.key.code == sf::Keyboard::W)     { if (snake.dir != sf::Vector2i(0,1))  snake.dir = {0,-1}; }
                    if (ev.key.code == sf::Keyboard::S)     { if (snake.dir != sf::Vector2i(0,-1)) snake.dir = {0,1}; }
                    if (ev.key.code == sf::Keyboard::A)     { if (snake.dir != sf::Vector2i(1,0))  snake.dir = {-1,0}; }
                    if (ev.key.code == sf::Keyboard::D)     { if (snake.dir != sf::Vector2i(-1,0)) snake.dir = {1,0}; }
                }
                if (gameOver && ev.key.code == sf::Keyboard::R) {
                    snake = resetGame();
                    food = spawnFood(snake);
                    moveDelay = 0.12f; acc = 0.f; gameOver = false; score = 0;
                }
            }
        }

        float dt = clock.restart().asSeconds();
        acc += dt;
        if (!gameOver && acc >= moveDelay) {
            acc -= moveDelay;
            snake.move();

            // wall collision
            auto h = snake.body.front();
            if (h.x < 0 || h.x >= COLS || h.y < 0 || h.y >= ROWS) {
                gameOver = true;
            }
            // self collision
            if (snake.hitsSelf()) gameOver = true;

            // eating food
            if (!gameOver && h == food) {
                snake.grow = true;
                score += 10;
                // speed up a bit
                moveDelay = std::max(0.03f, moveDelay * 0.95f);
                food = spawnFood(snake);
            }
        }

        // render
        window.clear(sf::Color(30,30,30));

        // draw food
        if (food.x >= 0) {
            blockShape.setPosition((float)(food.x*BLOCK), (float)(food.y*BLOCK));
            blockShape.setFillColor(sf::Color::Red);
            window.draw(blockShape);
        }

        // draw snake
        bool head = true;
        for (auto &p : snake.body) {
            blockShape.setPosition((float)(p.x*BLOCK), (float)(p.y*BLOCK));
            blockShape.setFillColor(head ? sf::Color::Green : sf::Color(0,180,0));
            window.draw(blockShape);
            head = false;
        }

        // HUD
        if (fontLoaded) {
            scoreText.setString("Score: " + std::to_string(score) + "  High: " + std::to_string(highscore));
            window.draw(scoreText);
        }

        if (gameOver) {
            if (score > highscore) { highscore = score; saveHighscore(highscore); }
            if (fontLoaded) {
                infoText.setString("Game Over! Press R to restart");
                window.draw(infoText);
            }
        }

        window.display();
    }

    return 0;
}
