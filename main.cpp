#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include <future>

const int HEIGHT_MAP = 7;
const int WIDTH_MAP = 7;

const std::map<int, sf::IntRect> tileTextureMap = {
    {1, sf::IntRect(0 * 64, 0, 64, 64)},
    {2, sf::IntRect(1 * 64, 0, 64, 64)},
    {3, sf::IntRect(2 * 64, 0, 64, 64)},
    {4, sf::IntRect(3 * 64, 0, 64, 64)},
    {5, sf::IntRect(4 * 64, 0, 64, 64)},
    {6, sf::IntRect(5 * 64, 0, 64, 64)}
};

enum class TileState 
{
    Idle,
    Moving,
    Removing,
    Appearing,
    Falling
};

class AnimationHandler 
{
public:
    AnimationHandler() :
        currentPos(0, 0),
        targetPos(0, 0),
        scale(1.0f),
        alpha(255.0f),
        state(TileState::Idle),
        fallSpeed(500.0f) // Скорость падения, настраивается
    {
    }

    void update(float deltaTime) 
    {
        if (state != TileState::Idle) 
        {
            // Обновляем анимацию только для активных тайлов
            switch (state) {
            case TileState::Moving: updateMoving(deltaTime); break;
            case TileState::Removing: updateRemoving(deltaTime); break;
            case TileState::Appearing: updateAppearing(deltaTime); break;
            case TileState::Falling: updateFalling(deltaTime); break;
            default: break;
            }
        }
    }

    void startMoving(const sf::Vector2f& newTarget) 
    {
        targetPos = newTarget;
        state = TileState::Moving;
    }

    void startRemoving() 
    {
        state = TileState::Removing;
        scale = 1.0f;
        alpha = 255.0f;
    }

    void startAppearing()
    {
        state = TileState::Appearing;
        scale = 0.0f;
        alpha = 0.0f;
    }

    void startFalling(const sf::Vector2f& newTarget) 
    {
        targetPos = newTarget;
        state = TileState::Falling;
    }

    void setPosition(float x, float y) 
    {
        currentPos = { x, y };
        targetPos = { x, y };
    }

    void setPosition(const sf::Vector2f& position) 
    {
        currentPos = position;
        targetPos = position;
    }

    void setScale(float scaleX, float scaleY) {
        this->scaleX = scaleX;
        this->scaleY = scaleY;
    }

    float getScaleX() const { return scaleX; }
    float getScaleY() const { return scaleY; }

    sf::Vector2f getPosition() const { return currentPos; }
    float getScale() const { return scale; }
    float getAlpha() const { return alpha; }
    TileState getState() const { return state; }


    //void setPosition(const sf::Vector2f& pos) { currentPos = pos; }
    void setTargetPosition(const sf::Vector2f& pos) { targetPos = pos; }
    bool isFinished() const { return state == TileState::Idle; } // Добавлено

private:
    void updateMoving(float deltaTime) 
    {
        sf::Vector2f delta = targetPos - currentPos;
        currentPos += delta * (2.0f * deltaTime); // Скорость движения 1.0, можно настроить
        if (std::abs(delta.x) < 2.0f && std::abs(delta.y) < 2.0f) {
            currentPos = targetPos;
            state = TileState::Idle;
        }
    }

    void updateRemoving(float deltaTime) 
    {
        scale = std::max(0.0f, scale - 2.0f * deltaTime); // Скорость исчезновения 0.5, можно настроить
        alpha = std::max(0.0f, alpha - 255.0f * 2.0f * deltaTime); // Скорость исчезновения альфы 0.5, можно настроить
        if (scale <= 0.0f) {
            scale = 1.0f; // Ensure it's exactly 0 to avoid rendering glitches
            alpha = 0.0f; // Ensure it's exactly 0
            state = TileState::Idle; // После удаления состояние в Idle, а не Appearing
        }
    }

    void updateAppearing(float deltaTime)
    {
        scale = std::min(1.0f, scale + 1.0f * deltaTime); // Скорость появления 1.0, можно настроить
        alpha = std::min(255.0f, alpha + 255.0f * 1.0f * deltaTime); // Скорость появления альфы 1.0, можно настроить
        if (scale >= 1.0f) 
        {
            scale = 1.0f; // Ensure it's exactly 1
            alpha = 255.0f; // Ensure it's exactly 255
            state = TileState::Idle;
        }
    }

    void updateFalling(float deltaTime) 
    {
        sf::Vector2f delta = targetPos - currentPos;
        currentPos.y += fallSpeed * deltaTime;  // Движение только по Y
        if (currentPos.y >= targetPos.y) 
        {  // Используем >=, чтобы избежать проскакивания цели
            currentPos = targetPos;
            state = TileState::Idle;
        }
    }

    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    float scale;
    float alpha;
    TileState state;
    float fallSpeed; // Скорость падения
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

class Tile 
{
public:
    Tile() : value(0) {
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
    }

    void setTexture(const sf::Texture& texture) 
    {
        sprite.setTexture(texture);
    }

    void setValue(int val) 
    {
        value = val;
    }

    void setPosition(float x, float y) 
    {
        // Учитываем точку привязки спрайта
        sprite.setPosition(x + sprite.getLocalBounds().width / 2, y + sprite.getLocalBounds().height / 2);
        animator.setPosition({ x, y });
        animator.setTargetPosition({ x, y });
    }

    void update(float deltaTime) {
        animator.update(deltaTime);

        if (isSelected) {
            // Анимация пульсации
            float time = selectTimer.getElapsedTime().asSeconds();
            float scale = 1.0f + 0.1f * sin(time * 10.0f); // Пульсация масштаба
            animator.setScale(scale, scale);

            // Случайное смещение для эффекта дрожи
            float shakeX = (rand() % 5 - 2) * 0.5f;
            float shakeY = (rand() % 5 - 2) * 0.5f;
            sprite.setPosition(animator.getPosition() + sf::Vector2f(shakeX, shakeY));
        }

        applyVisualState();
    }

    void applyVisualState() 
    {
        sprite.setPosition(animator.getPosition());
        sprite.setScale(animator.getScaleX(), animator.getScaleY());
        sprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(animator.getAlpha())));
    }

    void startMoving(const sf::Vector2f& target) 
    {
        animator.startMoving(target);
    }

    void startRemoving() 
    {
        animator.startRemoving();
    }

    void startAppearing() 
    {
        animator.startAppearing();
    }

    void startSelectAnimation() {
        isSelected = true;
        selectTimer.restart();
    }

    void stopSelectAnimation() {
        isSelected = false;
        animator.setScale(1.0f, 1.0f);
        sprite.setPosition(animator.getPosition());
    }

    AnimationHandler& getAnimator() { return animator; }
    bool isRemoving() const { return animator.getState() == TileState::Removing; }
    bool isFalling() const { return animator.getState() == TileState::Falling; }

    int getValue() const { return value; }
    TileState getState() const { return animator.getState(); }
    sf::Vector2f getPosition() const { return animator.getPosition(); }
    void startFalling(const sf::Vector2f& target) 
    {
        animator.startFalling(target);
    }
    void setFallPosition(const sf::Vector2f& target) 
    {
        animator.setPosition(target);
    }
    
    sf::Sprite sprite;
    bool isSelected = false;
    sf::Clock selectTimer;

private:
    int value;
    AnimationHandler animator;
};

struct LastMove 
{
    int selectedX = -1, selectedY = -1;
    int targetX = -1, targetY = -1;
};

void revertSwap(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, LastMove& lastMove) {
    // Откатываем обмен
    std::swap(tiles[lastMove.selectedY][lastMove.selectedX], tiles[lastMove.targetY][lastMove.targetX]);
    std::swap(tileMap[lastMove.selectedY][lastMove.selectedX], tileMap[lastMove.targetY][lastMove.targetX]);

    // Возвращаем тайлы на их исходные позиции
    tiles[lastMove.selectedY][lastMove.selectedX].startMoving(tiles[lastMove.selectedY][lastMove.selectedX].getPosition());
    tiles[lastMove.targetY][lastMove.targetX].startMoving(tiles[lastMove.targetY][lastMove.targetX].getPosition());
}

bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int startX, int startY) 
{
    return mouseX >= startX + x * squareSize && mouseX < startX + (x + 1) * squareSize &&
        mouseY >= startY + y * squareSize && mouseY < startY + (y + 1) * squareSize;
}

void updateTileSprite(Tile& tile, const sf::Texture& clothesTexture) 
{
    if (tile.getValue() >= 1 && tile.getValue() <= 6) 
    {
        if (tile.sprite.getTexture() != &clothesTexture) 
        {
            tile.sprite.setTexture(clothesTexture);
        }
        auto it = tileTextureMap.find(tile.getValue());
        if (it != tileTextureMap.end()) 
        {
            tile.sprite.setTextureRect(it->second);
        }
        else 
        {
            std::cerr << "Warning: No texture info for value " << tile.getValue() << std::endl;
        }
    }
    else if (tile.getValue() == 9) 
    {
        tile.sprite.setColor(sf::Color::Transparent);
    }
}

// Поиск совпадений
std::vector<std::vector<bool>> findMatches(const std::vector<std::vector<int>>& tileMap)
{
    int height = tileMap.size();
    int width = tileMap[0].size();
    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));

    // Горизонтальные совпадения
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width - 2; ++x) 
        {
            int value = tileMap[y][x];
            if (value != 0 && value != 9 &&
                value == tileMap[y][x + 1] &&
                value == tileMap[y][x + 2]) {
                toRemove[y][x] = true;
                toRemove[y][x + 1] = true;
                toRemove[y][x + 2] = true;
            }
        }
    }

    // Вертикальные совпадения
    for (int x = 0; x < width; ++x) 
    {
        for (int y = 0; y < height - 2; ++y) 
        {
            int value = tileMap[y][x];
            if (value != 0 && value != 9 &&
                value == tileMap[y + 1][x] &&
                value == tileMap[y + 2][x]) 
            {
                toRemove[y][x] = true;
                toRemove[y + 1][x] = true;
                toRemove[y + 2][x] = true;
            }
        }
    }

    return toRemove;
}

// Удаление совпадений
void removeMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, const std::vector<std::vector<bool>>& toRemove) 
{
    int height = tileMap.size();
    int width = tileMap[0].size();

    for (int y = 0; y < height; ++y) 
    {
        for (int x = 0; x < width; ++x) 
        {
            if (toRemove[y][x]) 
            {
                tiles[y][x].startRemoving();
                tileMap[y][x] = 0;
            }
        }
    }
}

// Применение гравитации (падение тайлов)
void applyGravity(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles,
    int width, int height, float tileSize, const sf::Texture& clothesTex,
    int startX, int startY) 
{
    for (int x = 0; x < width; ++x) 
    {
        int writeY = height - 1;
        for (int y = height - 1; y >= 0; --y) 
        {
            if (tileMap[y][x] != 0 && tileMap[y][x] != 9) 
            {
                if (writeY != y) 
                {
                    int value = tileMap[y][x];
                    tileMap[writeY][x] = value;
                    tileMap[y][x] = 0;

                    // Обновляем новый тайл
                    tiles[writeY][x].setValue(value);
                    updateTileSprite(tiles[writeY][x], clothesTex);

                    // Запускаем анимацию падения из старой позиции
                    sf::Vector2f startPos(startX + x * tileSize, startY + y * tileSize);
                    sf::Vector2f targetPos(startX + x * tileSize, startY + writeY * tileSize);
                    tiles[writeY][x].setPosition(startPos.x, startPos.y);  // Используем перегруженный метод
                    tiles[writeY][x].startFalling(targetPos);
                }
                writeY--;
            }
            else if (tileMap[y][x] == 9) 
            {
                writeY = y - 1;
            }
        }
    }
}

const std::vector<sf::Vector2i> corners = {
        {0,0}, {0,1}, {0,5}, {0,6}, {1,0}, {1,6},
        {5,0}, {5,6}, {6,0}, {6,1}, {6,5}, {6,6}
};

void fillEmptyTiles(std::vector<std::vector<int>>& tileMap,
    std::vector<std::vector<Tile>>& tiles,
    int width, int height,
    float tileSize,
    const sf::Texture& clothesTex,
    int startX, int startY,
    const std::vector<sf::Vector2i>& corners) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    for (int x = 0; x < width; ++x) 
    {
        for (int y = 0; y < height; ++y) 
        {
            // Заполняем только пустые ячейки (0), которые НЕ являются угловыми (9)
            if (tileMap[y][x] == 0) {
                bool isCorner = false;
                for (const auto& corner : corners) 
                {
                    if (corner.x == x && corner.y == y) 
                    {
                        isCorner = true;
                        break;
                    }
                }

                if (!isCorner) 
                {
                    int newValue = distrib(gen);
                    tileMap[y][x] = newValue;
                    tiles[y][x].setValue(newValue);

                    // Начальная позиция сверху за экраном
                    sf::Vector2f startPosition(startX + x * tileSize, startY - tileSize);
                    tiles[y][x].setFallPosition(startPosition);

                    // Целевая позиция
                    sf::Vector2f targetPosition(startX + x * tileSize, startY + y * tileSize);
                    tiles[y][x].startFalling(targetPosition);

                    updateTileSprite(tiles[y][x], clothesTex);
                }
                else 
                {
                    // Убедимся, что угловые элементы остаются 9
                    tileMap[y][x] = 9;
                    tiles[y][x].setValue(9);
                    updateTileSprite(tiles[y][x], clothesTex);
                }
            }
        }
    }
}


bool hasMatches(const std::vector<std::vector<int>>& tileMap) 
{
    // Оптимизированная проверка без полного поиска
    for (int y = 0; y < HEIGHT_MAP; y++) 
    {
        for (int x = 0; x < WIDTH_MAP - 2; x++) 
        {
            if (tileMap[y][x] == tileMap[y][x + 1] &&
                tileMap[y][x] == tileMap[y][x + 2] &&
                tileMap[y][x] != 0 && tileMap[y][x] != 9) return true;
        }
    }

    for (int x = 0; x < WIDTH_MAP; x++) 
    {
        for (int y = 0; y < HEIGHT_MAP - 2; y++) 
        {
            if (tileMap[y][x] == tileMap[y + 1][x] &&
                tileMap[y][x] == tileMap[y + 2][x] &&
                tileMap[y][x] != 0 && tileMap[y][x] != 9) return true;
        }
    }
    return false;
}

// Основная функция для обработки совпадений
void findAndReplaceMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, int width, int height, float tileSize, const sf::Texture& clothesTex, int startX, int startY, const std::vector<sf::Vector2i>& corners)
{
    while (true) 
    {
        auto toRemove = findMatches(tileMap);
        bool hasMatches = false;
        for (const auto& row : toRemove)
            if (std::any_of(row.begin(), row.end(), [](bool v) {return v; }))
                hasMatches = true;

        if (!hasMatches) break;

        removeMatches(tileMap, tiles, toRemove);
        applyGravity(tileMap, tiles, width, height, tileSize, clothesTex, startX, startY);
        fillEmptyTiles(tileMap, tiles, width, height, tileSize, clothesTex, startX, startY, corners);

        std::cout << "new map:" << std::endl; // Выводим в консоль метку "old map:"
        // Выводим результат в консоль (для проверки)
        for (int y = 0; y < HEIGHT_MAP; ++y)
        {
            for (int x = 0; x < WIDTH_MAP; ++x)
            {
                std::cout << tileMap[y][x] << " ";
            }
            std::cout << std::endl;
        }
    }
}

// Поиск возможных совпадений
// Поиск возможных совпадений (работает с копией матрицы)
std::vector<std::vector<sf::Vector2i>> findPossibleMatches(const std::vector<std::vector<int>>& tileMap) 
{
    std::vector<std::vector<sf::Vector2i>> matches;
    int height = tileMap.size();
    int width = tileMap[0].size();

    // Создаем копию матрицы для тестирования
    auto tempMap = tileMap;

    // Проверка горизонтальных свапов
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width - 1; x++) 
        {
            if (tempMap[y][x] == 9 || tempMap[y][x + 1] == 9) continue; // Пропускаем угловые

            std::swap(tempMap[y][x], tempMap[y][x + 1]);
            if (hasMatches(tempMap))
            {
                matches.push_back({ {x, y}, {x + 1, y} });
            }
            std::swap(tempMap[y][x], tempMap[y][x + 1]); // Возвращаем обратно
        }
    }

    // Проверка вертикальных свапов
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height - 1; y++) 
        {
            if (tempMap[y][x] == 9 || tempMap[y + 1][x] == 9) continue;

            std::swap(tempMap[y][x], tempMap[y + 1][x]);
            if (hasMatches(tempMap)) 
            {
                matches.push_back({ {x, y}, {x, y + 1} });
            }
            std::swap(tempMap[y][x], tempMap[y + 1][x]);
        }
    }

    return matches;
}

// Перемешивание доски
void shuffleBoard(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles,
    const sf::Texture& clothesTex, int startX, int startY, int squareSize) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    do 
    {
        // Перемешиваем все не угловые тайлы
        for (int y = 0; y < HEIGHT_MAP; y++) 
        {
            for (int x = 0; x < WIDTH_MAP; x++) 
            {
                if (tileMap[y][x] != 9) 
                {
                    tileMap[y][x] = distrib(gen);
                    tiles[y][x].setValue(tileMap[y][x]);
                    updateTileSprite(tiles[y][x], clothesTex);
                }
            }
        }
    } while (hasMatches(tileMap) || !findPossibleMatches(tileMap).empty());
}

sf::Color hexToColor(const std::string& hexColor) 
{
    std::string color = hexColor;
    if (color[0] == '#') color.erase(0, 1);
    if (color.length() != 6) return sf::Color::Black;

    unsigned int r, g, b;
    std::stringstream ss;
    ss << std::hex << color.substr(0, 2); ss >> r; ss.clear();
    ss << std::hex << color.substr(2, 2); ss >> g; ss.clear();
    ss << std::hex << color.substr(4, 2); ss >> b;

    return sf::Color(r, g, b);
}

enum class GameState
{
    Playing,
    Swapping,
    RemovingMatches,
    ApplyingGravity,
    FillingEmptyTiles
};


int main() 
{
    setlocale(LC_ALL, "RUSSIAN");
    const int squareSize = 84;
    sf::RenderWindow window(sf::VideoMode(1366, 770), "Big wash");

    // Load textures
    sf::Texture background, leftPanelTex, mainPanelTex, clothesTex;
    if (!background.loadFromFile("pictures/background.png") ||
        !leftPanelTex.loadFromFile("pictures/panel_L_arrows.png") ||
        !mainPanelTex.loadFromFile("pictures/main_panel.png") ||
        !clothesTex.loadFromFile("pictures/clothes.png"))
    {
        std::cerr << "Failed to load textures" << std::endl;
        return EXIT_FAILURE;
    }

    // Setup sprites
    sf::Sprite backgroundSprite(background);
    sf::Sprite leftPanel(leftPanelTex);
    sf::Sprite mainPanel(mainPanelTex);
    leftPanel.setPosition(10, 100);
    mainPanel.setPosition(365, 67);

    sf::Vector2i selectedTile = { -1, -1 }; // Координаты выделенного тайла

    // Initialize game grid
    std::vector<std::vector<int>> tileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP, 0));
    std::vector<std::vector<Tile>> tiles(HEIGHT_MAP, std::vector<Tile>(WIDTH_MAP));
    

    // Initialize random generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    // Setup initial grid
    for (const auto& pos : corners) tileMap[pos.y][pos.x] = 9;

    const int startX = 397;
    const int startY = 99;
    for (int y = 0; y < HEIGHT_MAP; ++y) 
    {
        for (int x = 0; x < WIDTH_MAP; ++x) 
        {
            if (tileMap[y][x] == 0) tileMap[y][x] = distrib(gen);
            tiles[y][x].setValue(tileMap[y][x]);
            tiles[y][x].setTexture(clothesTex);
            tiles[y][x].setPosition(startX + x * squareSize, startY + y * squareSize);
            updateTileSprite(tiles[y][x], clothesTex);
        }
    }

    std::cout << "old map:" << std::endl; // Выводим в консоль метку "old map:"
    // Выводим результат в консоль (для проверки)
    for (int y = 0; y < HEIGHT_MAP; ++y)
    {
        for (int x = 0; x < WIDTH_MAP; ++x)
        {
            std::cout << tileMap[y][x] << " ";
        }
        std::cout << std::endl;
    }

    auto future = std::async(std::launch::async, findMatches, std::ref(tileMap));
    auto toRemove = future.get();

    // Game state
    LastMove lastMove;
    bool dragging = false;
    int selectedX = -1, selectedY = -1;
    sf::Clock clock;
    sf::Font font;
    sf::Text text;
    GameState gameState = GameState::Playing; // Initial game state
    sf::Clock idleTimer; //таймер бездействия
    std::vector<sf::Vector2i> highlightedTiles; //Подсвеченыые тайлы
    bool isBoardValid = true; //Флаг валидности доски
    idleTimer.restart(); // Запускаем таймер при старте

    if (!font.loadFromFile("fonts/fredfredburgerheadline.otf")) 
    {
        std::cerr << "Failed to load font" << std::endl;
        return EXIT_FAILURE;
    }
    text.setFont(font);
    text.setCharacterSize(60);
    text.setFillColor(sf::Color::White);
    text.setPosition(90, 523);
    text.setOutlineThickness(3);
    text.setOutlineColor(hexToColor("#6b46d5"));

    while (window.isOpen())
    {
        sf::Event event;
        sf::Time deltaTime = clock.restart();

        // Проверка на совпадения
        if (hasMatches(tileMap))
        {
            findAndReplaceMatches(tileMap, tiles, WIDTH_MAP, HEIGHT_MAP, squareSize, clothesTex, startX, startY, corners);
        }

        if (idleTimer.getElapsedTime().asSeconds() > 10.0f && isBoardValid) 
        {
            auto possibleMatches = findPossibleMatches(tileMap);

            if (!possibleMatches.empty())
            {
                // Берем первый найденный возможный ход
                highlightedTiles = possibleMatches[0];

                // Подсвечиваем тайлы
                for (auto& pos : highlightedTiles) 
                {
                    tiles[pos.y][pos.x].startSelectAnimation();
                }
            }
            else 
            {
                // Перемешиваем доску если нет возможных ходов
                shuffleBoard(tileMap, tiles, clothesTex, startX, startY, squareSize);
                isBoardValid = false;
            }
            idleTimer.restart();
        }

        if (hasMatches(tileMap))
        {
            findAndReplaceMatches(tileMap, tiles, WIDTH_MAP, HEIGHT_MAP, squareSize, clothesTex, startX, startY, corners);
        }

        if (!highlightedTiles.empty() && gameState == GameState::Playing) 
        {
            // Сбрасываем подсветку
            for (auto& pos : highlightedTiles) 
            {
                tiles[pos.y][pos.x].stopSelectAnimation();
            }
            highlightedTiles.clear();
            isBoardValid = true;
        }

        // Обработка событий
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && gameState == GameState::Playing)
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                for (int y = 0; y < HEIGHT_MAP; ++y)
                {
                    for (int x = 0; x < WIDTH_MAP; ++x)
                    {
                        if (isSquareSelected(x, y, mousePos.x, mousePos.y, squareSize, startX, startY))
                        {
                            // Сбрасываем предыдущее выделение
                            if (selectedTile.x != -1) {
                                tiles[selectedTile.y][selectedTile.x].stopSelectAnimation();
                            }

                            selectedTile = { x, y };
                            tiles[y][x].startSelectAnimation();
                            selectedX = x;
                            selectedY = y;
                            dragging = true;
                        }
                    }
                }
            }

            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging && gameState == GameState::Playing)
            {
                if (selectedTile.x != -1)
                {
                    tiles[selectedTile.y][selectedTile.x].stopSelectAnimation();
                    selectedTile = { -1, -1 };
                }

                dragging = false;
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int targetX = (mousePos.x - startX) / squareSize;
                int targetY = (mousePos.y - startY) / squareSize;

                if (targetX >= 0 && targetX < WIDTH_MAP && targetY >= 0 && targetY < HEIGHT_MAP &&
                    (abs(targetX - selectedX) + abs(targetY - selectedY) == 1))
                {
                    // Проверяем, что ни исходный, ни целевой тайл не являются угловыми
                    bool isSelectedCorner = false;
                    bool isTargetCorner = false;

                    for (const auto& corner : corners)
                    {
                        if (corner.x == selectedX && corner.y == selectedY) isSelectedCorner = true;
                        if (corner.x == targetX && corner.y == targetY) isTargetCorner = true;
                    }

                    if (!isSelectedCorner && !isTargetCorner)
                    {
                        lastMove = { selectedX, selectedY, targetX, targetY };

                        // Swap tiles
                        std::swap(tiles[selectedY][selectedX], tiles[targetY][targetX]);
                        std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]);

                        // Set animation positions
                        tiles[selectedY][selectedX].startMoving(tiles[targetY][targetX].getPosition());
                        tiles[targetY][targetX].startMoving(tiles[selectedY][selectedX].getPosition());
                        gameState = GameState::Swapping; // Set game state to swapping

                        // Check matches
                        if (!hasMatches(tileMap))
                        {
                            // Если совпадений нет, откатываем обмен
                            revertSwap(tileMap, tiles, lastMove);
                        }
                        else
                        {
                            // Если есть совпадения, продолжаем обработку
                            findAndReplaceMatches(tileMap, tiles, WIDTH_MAP, HEIGHT_MAP, squareSize, clothesTex, startX, startY, corners);
                        }
                    }
                }
            }
        }

        // Game state machine
        switch (gameState)
        {
        case GameState::Playing:
            // Nothing to do here
            break;
        case GameState::Swapping:
            // Check if swapping animation is complete
            if (tiles[lastMove.selectedY][lastMove.selectedX].getAnimator().isFinished() &&
                tiles[lastMove.targetY][lastMove.targetX].getAnimator().isFinished()) 
            {
                gameState = GameState::RemovingMatches; // Move to removing state
            }
            break;
        case GameState::RemovingMatches:
        {
            // Find matches and start removing animations
            toRemove = findMatches(tileMap);
            removeMatches(tileMap, tiles, toRemove);
            gameState = GameState::ApplyingGravity;
            break;
        }
        case GameState::ApplyingGravity: 
            {
            bool allFinished = true;
            for (auto& row : tiles)
                for (auto& tile : row)
                    if (tile.getState() == TileState::Falling)
                        allFinished = false;

            if (allFinished)
                gameState = GameState::FillingEmptyTiles;
            break;
        }
        case GameState::FillingEmptyTiles: 
            {
            bool fallingFinished = true;
            for (int y = 0; y < HEIGHT_MAP; ++y) 
            {
                for (int x = 0; x < WIDTH_MAP; ++x) 
                {
                    if (tiles[y][x].isFalling() && !tiles[y][x].getAnimator().isFinished()) 
                    {
                        fallingFinished = false;
                        break;
                    }
                }
                if (!fallingFinished) break;
            }

            if (fallingFinished) 
            {
                // Проверяем совпадения после заполнения
                if (hasMatches(tileMap)) 
                {
                    gameState = GameState::RemovingMatches;
                }
                else {
                    gameState = GameState::Playing;
                }
            }
            break;
        }
        }

        // Обновляем и рисуем элементы одежды на игровом поле
        for (int i = 0; i < HEIGHT_MAP; ++i) 
        {
            for (int j = 0; j < WIDTH_MAP; ++j) 
            {
                tiles[i][j].update(deltaTime.asSeconds()); // Обновляем состояние тайла
                updateTileSprite(tiles[i][j], clothesTex);
            }
        }

        window.clear();
        window.draw(backgroundSprite);
        window.draw(mainPanel);
        window.draw(leftPanel);

        for (int i = 0; i < HEIGHT_MAP; ++i) 
        {
            for (int j = 0; j < WIDTH_MAP; ++j) 
            {
                window.draw(tiles[i][j].sprite); // Рисуем спрайт тайла
            }
        }

        window.display();
    }
    return 0;
}