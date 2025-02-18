#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>  // Для stringstream
#include <iomanip>  // Для setw и setfill
#include <map>

const int HEIGHT_MAP = 7; // Определяем константу для высоты игрового поля (7 клеток)
const int WIDTH_MAP = 7;  // Определяем константу для ширины игрового поля (7 клеток)

// Объявляем map, который связывает значения тайлов с информацией о текстуре
const std::map<int, sf::IntRect> tileTextureMap = {
    {1, sf::IntRect(0 * 64, 0, 64, 64)},  // tile.value = 1
    {2, sf::IntRect(1 * 64, 0, 64, 64)},  // tile.value = 2
    {3, sf::IntRect(2 * 64, 0, 64, 64)},  // tile.value = 3
    {4, sf::IntRect(3 * 64, 0, 64, 64)},  // tile.value = 4
    {5, sf::IntRect(4 * 64, 0, 64, 64)},  // tile.value = 5
    {6, sf::IntRect(5 * 64, 0, 64, 64)}   // tile.value = 6
};

enum class TileState {
    Idle,
    Moving,
    Removing,
    Appearing
};

class Tile 
{
public:
    Tile() : scale(1.0f), alpha(255.0f), targetScale(1.0f), moving(false), removing(false), appearing(false), scaling(false), value(0), state(TileState::Appearing) {}

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
        currentPos.x = x;
        currentPos.y = y;
        targetPos = currentPos;
    }

    // Этот метод теперь обновляет текстуру спрайта, а не только устанавливает цвет
    void updateSprite(const sf::Texture& clothesTexture)
    {
        if (value >= 1 && value <= 6) {
            if (sprite.getTexture() != &clothesTexture)
            {
                sprite.setTexture(clothesTexture);
            }

            const auto it = tileTextureMap.find(value);
            if (it != tileTextureMap.end()) 
            {
                sprite.setTextureRect(it->second);
                sprite.setColor(sf::Color::White); // Восстанавливаем видимость
            }
            else 
            {
                std::cerr << "Warning: No texture information found for tile.value = " << value << std::endl;
                sprite.setColor(sf::Color::Transparent); // Или установите текстуру по умолчанию
            }
        }
        else if (value == 9 || value == 0) 
        {
            sprite.setColor(sf::Color::Transparent);
        }
        else if (removing) 
        {
            sprite.setColor(sf::Color::Transparent);
        }
    }

    void update(float deltaTime)
    {
        switch (state)
        {
        case TileState::Moving:
        {
            sf::Vector2f delta = targetPos - currentPos;
            currentPos += delta * (1.0f * deltaTime);
            if (std::abs(delta.x) < 1.0f && std::abs(delta.y) < 1.0f)
            {
                currentPos = targetPos;
                state = TileState::Idle;
            }
            break;
        }
        case TileState::Removing:
        {
            scale = std::max(0.0f, scale - 0.5f * deltaTime);
            alpha = std::max(0.0f, alpha - 255.0f * 0.5f * deltaTime);
            if (scale <= 0.0f)
            {
                state = TileState::Appearing;
                value = 0;
            }
            break;
        }
        case TileState::Appearing:
        {
            scale = std::min(1.0f, scale + 1.0f * deltaTime);
            alpha = std::min(255.0f, alpha + 255.0f * 1.0f * deltaTime);
            if (scale >= 1.0f)
            {
                state = TileState::Idle;
            }
            break;
        }
        default: break;
        }
        sprite.setPosition(currentPos);
        sprite.setScale(scale, scale);
        sprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
    }

    int getValue() const { return value; }
    TileState getState() const { return state; }

public:
    sf::Sprite sprite;
    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    sf::Vector2f originalPos;
    float scale;
    float alpha;
    float targetScale;
    bool moving;
    bool removing;
    bool appearing;
    bool scaling;
    bool returning;
    int value;
    float shakeIntensity;
    float appearAnimationSpeed = 1.0f; // Глобальная переменная
    TileState state; // Использование enum class TileState
};

//Добавляем структуру для хранения последнего перемещения
struct LastMove {
    int selectedX = -1, selectedY = -1;
    int targetX = -1, targetY = -1;
};

LastMove lastMove; // Глобальная переменная для хранения последнего перемещения

bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int startX, int startY) //проверяет в какой квадрат указывает мышь
{
    return mouseX >= startX + x * squareSize && mouseX < startX + (x + 1) * squareSize &&
        mouseY >= startY + y * squareSize && mouseY < startY + (y + 1) * squareSize;
}

void updateTileSprite(Tile& tile, const sf::Texture& clothesTexture)
{
    if (tile.value >= 1 && tile.value <= 6)
    {
        // Устанавливаем текстуру для одежды
        if (tile.sprite.getTexture() != &clothesTexture)
        {
            tile.sprite.setTexture(clothesTexture);
        }

        // Используем map для получения координат текстуры
        auto it = tileTextureMap.find(tile.value);
        if (it != tileTextureMap.end())
        {
            tile.sprite.setTextureRect(it->second); // it->second - это значение (sf::IntRect)
        }
        else
        {
            // Обработка случая, когда для данного значения tile.value нет информации о текстуре
            // Например, можно установить текстуру по умолчанию или вывести предупреждение
            std::cerr << "Warning: No texture information found for tile.value = " << tile.value << std::endl;
        }
        tile.sprite.setColor(sf::Color::White); // Восстанавливаем видимость
    }
    else if (tile.value == 9)
    {
        // Угловые элементы: скрываем или устанавливаем другую текстуру
        tile.sprite.setColor(sf::Color::Transparent);
    }
}

void findAndReplaceMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles)
{
    // srand(time(0));  // Инициализация генератора случайных чисел - ВЫНЕСТИ ЗА ПРЕДЕЛЫ ФУНКЦИИ, ИНАЧЕ БУДЕТ ВСЕГДА ОДИНАКОВО

    int height = tileMap.size();
    if (height == 0) return;
    int width = tileMap[0].size();
    if (width == 0) return;

    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));

    // Поиск горизонтальных совпадений (игнорируя 0 и 9)
    for (int y = 0; y < height; ++y)
    {
        int start = 0;
        int matchLength = 1;
        for (int x = 1; x <= width; ++x)
        {
            if (x < width && tileMap[y][x] == tileMap[y][x - 1] && tileMap[y][x] != 0 && tileMap[y][x] != 9)
            {
                matchLength++;
            }
            else {
                if (matchLength >= 3)
                {
                    for (int k = x - matchLength; k < x; ++k)
                    {
                        toRemove[y][k] = true;
                    }
                }

                start = x;
                matchLength = 1;
            }
        }
    }

    // Поиск вертикальных совпадений (игнорируя 0 и 9)
    for (int x = 0; x < width; ++x)
    {
        int start = 0;
        int matchLength = 1;
        for (int y = 1; y <= height; ++y)
        {
            if (y < height && tileMap[y][x] == tileMap[y - 1][x] && tileMap[y][x] != 0 && tileMap[y][x] != 9)
            {
                matchLength++;
            }
            else
            {
                if (matchLength >= 3)
                {
                    for (int k = y - matchLength; k < y; ++k)
                    {
                        toRemove[k][x] = true;
                    }
                }
                start = y;
                matchLength = 1;
            }
        }
    }

    // Обновление столбцов с учетом 9 и замена нулей
    for (int x = 0; x < width; ++x)
    {
        std::vector<int> splitIndices;
        splitIndices.push_back(-1); // Начальная граница
        for (int y = 0; y < height; ++y)
        {
            if (tileMap[y][x] == 9) {
                splitIndices.push_back(y);
            }
        }
        splitIndices.push_back(height); // Конечная граница

        for (int i = 0; i < splitIndices.size() - 1; ++i)
        {
            int segStart = splitIndices[i] + 1;
            int segEnd = splitIndices[i + 1];
            int segLen = segEnd - segStart;
            if (segLen <= 0) continue;

            std::vector<int> survivingElements;
            for (int y = segStart; y < segEnd; ++y)
            {
                if (!toRemove[y][x])
                {
                    survivingElements.push_back(tileMap[y][x]);
                }
                else
                {
                    // ***ВАЖНО: Устанавливаем состояние Removing для Tile***
                    tiles[y][x].state = TileState::Removing;
                    // Optionally, set a target value for the tile after it's removed (e.g., 0)
                    tiles[y][x].value = 0;
                }
            }

            int removedCount = segLen - survivingElements.size();
            int writeY = segStart;

            // Заполняем удаленные позиции нулями
            for (int j = 0; j < removedCount; ++j, ++writeY)
            {
                tileMap[writeY][x] = 0;
            }

            // Записываем выжившие элементы
            for (int val : survivingElements)
            {
                if (writeY >= segEnd) break;
                tileMap[writeY][x] = val;
                ++writeY;
            }

            // Заменяем нули на случайные числа 1-6
            for (int y = segStart; y < segEnd; ++y)
            {
                if (tileMap[y][x] == 0)
                {
                    // Вместо прямого присваивания значения, используем состояние Appearing
                    tileMap[y][x] = rand() % 6 + 1;
                    tiles[y][x].value = tileMap[y][x]; // Обновляем значение в Tile
                    tiles[y][x].state = TileState::Appearing; // Устанавливаем состояние Appearing
                }
            }
        }
    }
}

bool hasMatches(const std::vector<std::vector<int>>& tileMap)
{
    // Реализация аналогична findAndReplaceMatches, но без модификации карты
    // Возвращает true, если есть хотя бы одно совпадение

    int height = tileMap.size();
    if (height == 0) return false;
    int width = tileMap[0].size();
    if (width == 0) return false;

    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));

    // Поиск горизонтальных совпадений (игнорируя 0 и 9)
    for (int y = 0; y < height; ++y)
    {
        int start = 0;
        int matchLength = 1;
        for (int x = 1; x <= width; ++x)
        {
            if (x < width && tileMap[y][x] == tileMap[y][x - 1] && tileMap[y][x] != 0 && tileMap[y][x] != 9)
            {
                matchLength++;
            }
            else {
                if (matchLength >= 3)
                {
                    for (int k = x - matchLength; k < x; ++k)
                    {
                        toRemove[y][k] = true;
                    }
                }

                start = x;
                matchLength = 1;
            }
        }
    }

    // Поиск вертикальных совпадений (игнорируя 0 и 9)
    for (int x = 0; x < width; ++x)
    {
        int start = 0;
        int matchLength = 1;
        for (int y = 1; y <= height; ++y)
        {
            if (y < height && tileMap[y][x] == tileMap[y - 1][x] && tileMap[y][x] != 0 && tileMap[y][x] != 9)
            {
                matchLength++;
            }
            else
            {
                if (matchLength >= 3)
                {
                    for (int k = y - matchLength; k < y; ++k)
                    {
                        toRemove[k][x] = true;
                    }
                }
                start = y;
                matchLength = 1;
            }
        }
    }

    for (const auto& row : toRemove) 
    {
        for (bool val : row) 
        {
            if (val) return true;
        }
    }
    return false;
}

// Функция для преобразования hex-цвета в sf::Color
sf::Color hexToColor(const std::string& hexColor) 
{
    // Удаляем символ '#' (если есть)
    std::string color = hexColor;
    if (color[0] == '#') 
    {
        color = color.substr(1);
    }

    // Проверяем длину строки
    if (color.length() != 6) 
    {
        std::cerr << "Ошибка: Неверный формат hex-цвета.  Должно быть 6 символов (без #)." << std::endl;
        return sf::Color::Black; // Возвращаем черный цвет по умолчанию
    }

    // Преобразуем hex-строку в целые числа
    int r, g, b;
    std::stringstream ss;
    ss << std::hex << color.substr(0, 2);
    ss >> r;
    ss.clear();
    ss << std::hex << color.substr(2, 2);
    ss >> g;
    ss.clear();
    ss << std::hex << color.substr(4, 2);
    ss >> b;

    // Создаем и возвращаем sf::Color
    return sf::Color(r, g, b);
}

int main()
{
    setlocale(LC_ALL, "RUSSIAN"); // Устанавливаем русскую локаль для корректного отображения текста

    const int squareSize = 84;
    const float animationSpeed = 0.15f;
    const float removeAnimationSpeed = 0.1f;

    sf::RenderWindow window(sf::VideoMode(1366, 770), "Big wash"); // Создаем окно SFML с разрешением 1366x770 и заголовком "Big wash"

    sf::Texture background; // Создаем текстуру для фона
    background.loadFromFile("pictures/background.png"); // Загружаем изображение фона из файла "pictures/background.png"

    sf::Sprite sprite(background); // Создаем спрайт (графический объект) для фона, используя загруженную текстуру
    sprite.setTextureRect(sf::IntRect(0, 0, 1366, 770)); // Устанавливаем область текстуры, которая будет отображаться (весь фон)
    sprite.setPosition(0, 0); // Устанавливаем позицию спрайта фона в левый верхний угол окна

    //добавляем панель с левой стороны экрана
    sf::Image leftSidePanelImage; // Создаем изображение для левой боковой панели
    if (!leftSidePanelImage.loadFromFile("pictures/panel_L_arrows.png")) 
    { // Загружаем изображение панели из файла, проверяем на ошибку
        std::cerr << "Error loading pictures/panel_L_arrows.png" << std::endl; // Выводим сообщение об ошибке в консоль, если загрузка не удалась
        return EXIT_FAILURE; // Завершаем программу с кодом ошибки
    }

    sf::Texture leftSidePanelTexture; // Создаем текстуру для левой боковой панели
    leftSidePanelTexture.loadFromImage(leftSidePanelImage); // Загружаем изображение в текстуру

    sf::Sprite leftSidePanelSprite; // Создаем спрайт для левой боковой панели
    leftSidePanelSprite.setTexture(leftSidePanelTexture); // Устанавливаем текстуру для спрайта
    leftSidePanelSprite.setTextureRect(sf::IntRect(0, 0, 175, 521)); // Устанавливаем область текстуры, которая будет отображаться (часть панели)
    leftSidePanelSprite.setPosition(10, 100); // Устанавливаем позицию спрайта панели

    //добавляем главное поле в центр экрана
    sf::Image mainPanelImage; // Создаем изображение для левой боковой панели
    if (!mainPanelImage.loadFromFile("pictures/main_panel.png")) 
    { // Загружаем изображение панели из файла, проверяем на ошибку
        std::cerr << "Error loading pictures/main_panel.png" << std::endl; // Выводим сообщение об ошибке в консоль, если загрузка не удалась
        return EXIT_FAILURE; // Завершаем программу с кодом ошибки
    }

    sf::Texture mainPanelTexture; // Создаем текстуру для поля
    mainPanelTexture.loadFromImage(mainPanelImage); // Загружаем изображение в текстуру

    sf::Sprite mainPanelSprite; // Создаем спрайт для левой боковой панели
    mainPanelSprite.setTexture(mainPanelTexture); // Устанавливаем текстуру для спрайта
    mainPanelSprite.setTextureRect(sf::IntRect(0, 0, 636, 636)); // Устанавливаем область текстуры, которая будет отображаться (часть панели)
    mainPanelSprite.setPosition(365, 67); // Устанавливаем позицию спрайта панели

    sf::Image clothes;
    clothes.loadFromFile("pictures/clothes.png");

    sf::Texture clothes1;
    clothes1.loadFromImage(clothes);
    
    sf::Sprite s_clothes;
    s_clothes.setTexture(clothes1);

    const int startX = 397;
    const int startY = 99;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    std::vector<std::vector<Tile>> tiles(HEIGHT_MAP, std::vector<Tile>(WIDTH_MAP));
    std::vector<std::vector<int>> tileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP));
    std::vector<std::vector<Tile>> grid;


    grid.resize(HEIGHT_MAP, std::vector<Tile>(WIDTH_MAP));
    tileMap.resize(HEIGHT_MAP, std::vector<int>(WIDTH_MAP));

    // Initialize corners with 9
    const std::vector<sf::Vector2i> corners = {
        {0,0}, {0,1}, {0,5}, {0,6},
        {1,0}, {1,6},
        {5,0}, {5,6},
        {6,0}, {6,1}, {6,5}, {6,6}
    };

    for (const auto& pos : corners) {
        tileMap[pos.y][pos.x] = 9;
    }

    // Заполняем оставшиеся ячейки случайными числами от 1 до 6
    for (int y = 0; y < HEIGHT_MAP; ++y) 
    {
        for (int x = 0; x < WIDTH_MAP; ++x) 
        {
            if (tileMap[y][x] == 0) {
                tileMap[y][x] = distrib(gen); // Генерация случайных значений
            }

            tiles[y][x].setValue(tileMap[y][x]); // Устанавливаем значение тайла
            tiles[y][x].setTexture(clothes1); // Задаем текстуру для спрайта тайла
            tiles[y][x].setPosition(startX + x * squareSize, startY + y * squareSize); // Устанавливаем позицию тайла
            tiles[y][x].originalPos = tiles[y][x].currentPos; // Запоминаем исходную позицию
            tiles[y][x].updateSprite(clothes1); // Первоначальное обновление спрайта
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

    sf::Clock clock;

    bool isAnimating = false; // Флаг, указывающий, выполняется ли анимация
    bool needsCheckMatches = false;
    bool dragging = false;
    int selectedX = -1, selectedY = -1;
    float animationProgress = 0.0f; // Текущий прогресс анимации (от 0.0 до 1.0)

    sf::Vector2f animationStartPos, animationEndPos; // Начальная и конечная позиции для анимации

    const int gridSize = 606; // Размер сетки игрового поля (в пикселях)

    sf::RectangleShape selectedSquare; // Создаем прямоугольник для отображения выбранной ячейки

    sf::Font font; // Создаем объект шрифта
    if (!font.loadFromFile("fonts/fredfredburgerheadline.otf")) { // Загружаем шрифт из файла, проверяем на ошибку
        std::cerr << "Ошибка загрузки шрифта fonts/fredfredburgerheadline.otf" << std::endl; // Выводим сообщение об ошибке, если загрузка не удалась
        return -1; // Возвращаем код ошибки из main (или другой функции, где это используется)
    }

    sf::Text text; // Создаем текстовый объект SFML для отображения текста на экране.
    text.setFont(font); // Устанавливаем шрифт для текстового объекта.
    text.setCharacterSize(60); // Устанавливаем размер шрифта (в пикселях).
    text.setFillColor(sf::Color::White); // Устанавливаем цвет текста (белый).
    text.setPosition(90, 523); // Устанавливаем позицию текста на экране (x=90, y=523).
    text.setOutlineThickness(3.0f); // Устанавливаем толщину обводки текста (3 пикселя).
    text.setOutlineColor(hexToColor("#6b46d5")); // Устанавливаем цвет обводки текста (используем функцию hexToColor для преобразования hex-кода в цвет).

    while (window.isOpen())
    {
        sf::Event event; // Создаем объект события для обработки событий окна
        sf::Time deltaTime = clock.restart();

        while (window.pollEvent(event)) // Цикл обработки событий (закрытия окна, нажатия клавиш, мыши и т.д.)
        {
            findAndReplaceMatches(tileMap, tiles); // Вызываем функцию для поиска и замены совпадений в tileMap (логика игры). ВЫЗЫВАЕТСЯ ОЧЕНЬ ЧАСТО, возможно, нужно переместить

            if (event.type == sf::Event::Closed) // Если событие - закрытие окна
            {
                window.close(); // Закрываем окно
            }

            // Обработка нажатия кнопки мыши
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) 
            {
                int mouseX = event.mouseButton.x;
                int mouseY = event.mouseButton.y;

                for (int i = 0; i < HEIGHT_MAP; ++i) 
                {
                    for (int j = 0; j < WIDTH_MAP; ++j) 
                    {
                        if (isSquareSelected(j, i, mouseX, mouseY, squareSize, startX, startY)) 
                        {
                            selectedX = j;
                            selectedY = i;
                            selectedSquare.setPosition(startX + j * squareSize, startY + i * squareSize);
                            dragging = true;

                            break;
                        }
                    }
                }
            }

            // Обработка отпускания кнопки мыши (завершение перетаскивания)
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging) 
            {
                int mouseX = event.mouseButton.x;
                int mouseY = event.mouseButton.y;

                int targetX = (mouseX - startX) / squareSize;
                int targetY = (mouseY - startY) / squareSize;

                if (targetX >= 0 && targetX < WIDTH_MAP &&
                    targetY >= 0 && targetY < HEIGHT_MAP &&
                    selectedX >= 0 && selectedX < WIDTH_MAP &&
                    selectedY >= 0 && selectedY < HEIGHT_MAP &&
                    (abs(targetX - selectedX) <= 1 && abs(targetY - selectedY) <= 1) &&
                    !(targetX == selectedX && targetY == selectedY))

                {
                    lastMove = { selectedX, selectedY, targetX, targetY };

                    // Сохраняем исходные позиции
                    tiles[selectedY][selectedX].originalPos = tiles[selectedY][selectedX].currentPos;
                    tiles[targetY][targetX].originalPos = tiles[targetY][targetX].currentPos;

                    // ***Важно: Обмен Tile объектами***
                    std::swap(tiles[selectedY][selectedX], tiles[targetY][targetX]);

                    // ***Важно: Обмен значениями в tileMap***
                    std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]); // Обмен значениями в tileMap

                    // Задаем целевые позиции для анимации (исходя из текущих позиций соседнего тайла)
                    sf::Vector2f tempTargetPos = tiles[selectedY][selectedX].currentPos;
                    tiles[selectedY][selectedX].targetPos = tiles[targetY][targetX].currentPos;
                    tiles[targetY][targetX].targetPos = tempTargetPos;

                    // Запускаем анимацию перемещения
                    tiles[selectedY][selectedX].state = TileState::Moving;
                    tiles[targetY][targetX].state = TileState::Moving;

                    // Проверяем совпадения после перемещения
                    auto tempMap = tileMap;
                    if (!hasMatches(tempMap))
                    {
                        // Если совпадений нет - возвращаем все обратно

                        // Swap back in tileMap
                        std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]);

                        //Swap the Tiles object to its initial place
                        std::swap(tiles[selectedY][selectedX], tiles[targetY][targetX]);

                        //Set the animation of "return"
                        tiles[selectedY][selectedX].targetPos = tiles[selectedY][selectedX].originalPos;
                        tiles[targetY][targetX].targetPos = tiles[targetY][targetX].originalPos;
                        tiles[selectedY][selectedX].state = TileState::Moving;
                        tiles[targetY][targetX].state = TileState::Moving;
                    }
                    else
                    {
                        findAndReplaceMatches(tileMap, tiles);
                    }
                }

                dragging = false;

                std::cout << "new map:" << std::endl;
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

        if (!isAnimating) {
            findAndReplaceMatches(tileMap, tiles);
            needsCheckMatches = false;
        }

        window.clear(); // Очищаем окно (заливаем черным цветом по умолчанию)

        window.draw(sprite); // Рисуем спрайт фона

        window.draw(mainPanelSprite); // Рисуем основной прямоугольник сетки

        window.draw(leftSidePanelSprite); // Рисуем спрайт левой панели

        // Рисуем элементы одежды на игровом поле
        for (int i = 0; i < HEIGHT_MAP; ++i)
        {
            for (int j = 0; j < WIDTH_MAP; ++j)
            {
                int tileIndex = tileMap[i][j] - 1; // Получаем индекс элемента одежды (учитываем, что значения в tileMap от 1 до 6)
                tiles[i][j].update(deltaTime.asSeconds()); // Обновляем состояние тайла

                if (tileIndex >= 0 && tileIndex < 6) // Проверяем, что индекс находится в допустимом диапазоне
                {
                    s_clothes.setTextureRect(sf::IntRect(tileIndex * 64, 0, 64, 64)); // Устанавливаем область текстуры для спрайта одежды (выбираем нужный элемент)
                    s_clothes.setPosition(startX + j * 84, startY + i * 84); // Устанавливаем позицию спрайта одежды
                    window.draw(tiles[i][j].sprite); // Рисуем спрайт тайла
                }
            }
        }

        window.display(); // Отображаем содержимое окна
    }

    return 0;
}