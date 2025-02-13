#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>


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

class Tile {
public:
    Tile() : scale(1.0f), alpha(255.0f), targetScale(1.0f), moving(false), removing(false), appearing(false), scaling(false), value(0), distrib(1, 6) {}

    void setTexture(const sf::Texture& texture) {
        sprite.setTexture(texture);
    }

    void setValue(int val) {
        value = val;
    }

    void setPosition(float x, float y) {
        currentPos.x = x;
        currentPos.y = y;
        targetPos = currentPos; // Целевая позиция совпадает с начальной
    }

    void updateSprite(const sf::Texture& clothesTexture) {
        if (value >= 1 && value <= 6)
        {
            // Устанавливаем текстуру для одежды
            if (sprite.getTexture() != &clothesTexture)
            {
                sprite.setTexture(clothesTexture);
            }

            // Используем map для получения координат текстуры
            const auto it = tileTextureMap.find(value);
            if (it != tileTextureMap.end())
            {
                sprite.setTextureRect(it->second); // it->second - это значение (sf::IntRect)
            }
            else
            {
                // Обработка случая, когда для данного значения tile.value нет информации о текстуре
                // Например, можно установить текстуру по умолчанию
                std::cerr << "Warning: No texture information found for tile.value = " << value << std::endl;
            }
            sprite.setColor(sf::Color::White); // Восстанавливаем видимость
        }
        else if (value == 9 || value == 0) // Добавили проверку для value == 0
        {
            // Угловые элементы и пустые клетки: скрываем или устанавливаем другую текстуру
            sprite.setColor(sf::Color::Transparent);
        }
        else if (removing) {
            sprite.setColor(sf::Color::Transparent);
        }
    }

    int getValue() const { return value; }
    void setRandomValue(std::mt19937& gen) { value = distrib(gen); }

public:
    sf::Sprite sprite;
    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    float scale;
    float alpha;
    float targetScale;
    bool moving;
    bool removing;
    bool appearing;
    bool scaling;
    int value;

private:
    std::uniform_int_distribution<> distrib;
};

//класс для многопоточной проверки
class MatchChecker {
public:
    MatchChecker(std::vector<std::vector<int>>& map,
        std::vector<std::vector<Tile>>& tiles,
        sf::Texture& texture)
        : tileMap(map), tiles(tiles), clothesTexture(texture),
        running(false), needsUpdate(false) {
    }

    void start() {
        if (!running) {
            running = true;
            worker = std::thread(&MatchChecker::checkLoop, this);
        }
    }

    void stop() {
        running = false;
        if (worker.joinable()) worker.join();
    }

    bool hasMatches() {
        std::lock_guard<std::mutex> lock(mtx);
        return !currentMatches.empty();
    }

    std::vector<std::vector<bool>> getMatches() {
        std::lock_guard<std::mutex> lock(mtx);
        return std::move(currentMatches);
    }

    void requestUpdate() {
        std::lock_guard<std::mutex> lock(mtx);
        needsUpdate = true;
    }

    static std::vector<std::vector<bool>> findMatches(const std::vector<std::vector<int>>& tileMap);

private:
    void checkLoop() {
        while (running) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (needsUpdate) {
                    currentMatches = MatchChecker::findMatches(tileMap); // Используем статический метод
                    needsUpdate = false;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::thread worker;
    std::atomic<bool> running;
    std::mutex mtx;
    bool needsUpdate;
    std::vector<std::vector<bool>> currentMatches;

    std::vector<std::vector<int>>& tileMap;
    std::vector<std::vector<Tile>>& tiles;
    sf::Texture& clothesTexture;
};

// Функция для поиска совпадений
std::vector<std::vector<bool>> MatchChecker::findMatches(const std::vector<std::vector<int>>& tileMap)
{
    int height = tileMap.size();
    int width = height > 0 ? tileMap[0].size() : 0;
    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));
    bool matchesFound = false;

    // Поиск горизонтальных совпадений
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width - 2; ++x)
        {
            int value = tileMap[y][x];
            if (value == 0 || value == 9) continue; // Пропускаем пустые и угловые элементы

            // Проверяем, есть ли три одинаковых элемента подряд
            if (tileMap[y][x + 1] == value && tileMap[y][x + 2] == value)
            {
                // Помечаем все три элемента для удаления
                toRemove[y][x] = true;
                toRemove[y][x + 1] = true;
                toRemove[y][x + 2] = true;
                matchesFound = true;

                // Проверяем, есть ли больше трех одинаковых элементов
                int k = x + 3;
                while (k < width && tileMap[y][k] == value)
                {
                    toRemove[y][k] = true;
                    k++;
                }
            }
        }
    }

    // Поиск вертикальных совпадений
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height - 2; ++y)
        {
            int value = tileMap[y][x];
            if (value == 0 || value == 9) continue; // Пропускаем пустые и угловые элементы

            // Проверяем, есть ли три одинаковых элемента подряд
            if (tileMap[y + 1][x] == value && tileMap[y + 2][x] == value)
            {
                // Помечаем все три элемента для удаления
                toRemove[y][x] = true;
                toRemove[y + 1][x] = true;
                toRemove[y + 2][x] = true;
                matchesFound = true;

                // Проверяем, есть ли больше трех одинаковых элементов
                int k = y + 3;
                while (k < height && tileMap[k][x] == value)
                {
                    toRemove[k][x] = true;
                    k++;
                }
            }
        }
    }

    return toRemove;
}

// Функция для удаления совпадений и обновления тайлов
bool removeMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, sf::Texture& clothesTexture, const std::vector<std::vector<bool>>& toRemove, MatchChecker& matchChecker)
{
    int height = tileMap.size();
    int width = height > 0 ? tileMap[0].size() : 0;

    // Удаляем помеченные элементы (заменяем на 0)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (toRemove[y][x]) {
                tileMap[y][x] = 0;
                tiles[y][x].value = 0;
                tiles[y][x].removing = true;
            }
        }
    }

    // Опускаем элементы вниз, сохраняя углы 9
    for (int x = 0; x < width; ++x) {
        int writeY = height - 1;
        for (int y = height - 1; y >= 0; --y) {
            // Пропускаем угловые 9
            if (tileMap[y][x] == 9) {
                if (y != writeY) {
                    // Если 9 не на своем месте, перемещаем его
                    tileMap[writeY][x] = 9;
                    tiles[writeY][x].value = 9;
                    tiles[writeY][x].targetPos = tiles[writeY][x].currentPos;
                }
                writeY--;
            }
            else if (tileMap[y][x] != 0) {
                // Перемещаем неугловые элементы
                tileMap[writeY][x] = tileMap[y][x];
                tiles[writeY][x].value = tileMap[y][x];
                tiles[writeY][x].moving = true;
                writeY--;
            }
        }

        std::random_device rd; // Создаем объект для получения случайных чисел из аппаратного источника (если доступен)
        std::mt19937 gen(rd()); // Создаем генератор случайных чисел Mersenne Twister, инициализируем его с помощью rd
        std::uniform_int_distribution<> distrib(1, 6); // Создаем распределение случайных чисел в диапазоне от 1 до 6 (включительно)

        // Заполняем оставшиеся нули, кроме углов
        for (int y = writeY; y >= 0; --y) {
            if (tileMap[y][x] == 0) {
                // Проверяем, не является ли позиция угловой
                bool isCorner = (y < 2 || y >= height - 2) && (x < 2 || x >= width - 2);
                if (!isCorner) {
                    tileMap[y][x] = distrib(gen);
                    tiles[y][x].value = tileMap[y][x];
                    tiles[y][x].appearing = true;
                    tiles[y][x].updateSprite(clothesTexture);
                }
            }
        }
    }

    return true;
}

// Обновленная функция findAndReplaceMatches
bool findAndReplaceMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, sf::Texture& clothesTexture, MatchChecker& matchChecker)
{
    // Находим совпадения
    std::vector<std::vector<bool>> toRemove = MatchChecker::findMatches(tileMap);

    // Если совпадений нет, возвращаем false
    bool matchesFound = false;
    for (const auto& row : toRemove)
    {
        for (bool remove : row)
        {
            if (remove)
            {
                matchesFound = true;
                break;
            }
        }
        if (matchesFound) break;
    }

    if (!matchesFound)
    {
        return false;
    }

    // Удаляем совпадения и обновляем тайлы
    return removeMatches(tileMap, tiles, clothesTexture, toRemove, matchChecker); // Используем toRemove
}

// Функция проверки, находится ли точка (mouseX, mouseY) внутри квадрата
bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int offsetX, int offsetY)
{
    int squareLeft = offsetX + x * squareSize;
    int squareTop = offsetY + y * squareSize;
    int squareRight = squareLeft + squareSize;
    int squareBottom = squareTop + squareSize;

    return (mouseX >= squareLeft && mouseX <= squareRight &&
        mouseY >= squareTop && mouseY <= squareBottom);
}

// Функция для преобразования hex-цвета в sf::Color
sf::Color hexToColor(const std::string& hexColor) {
    // Удаляем символ '#' (если есть)
    std::string color = hexColor;
    if (color[0] == '#') {
        color = color.substr(1);
    }

    // Проверяем длину строки
    if (color.length() != 6) {
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
    if (!background.loadFromFile("pictures/background.png"))
    {
        std::cerr << "Error loading background.png" << std::endl;
        return EXIT_FAILURE;
    } // Загружаем изображение фона из файла "pictures/background.png"

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
    if (!mainPanelImage.loadFromFile("pictures/main_panel.png")) { // Загружаем изображение панели из файла, проверяем на ошибку
        std::cerr << "Error loading pictures/main_panel.png" << std::endl; // Выводим сообщение об ошибке в консоль, если загрузка не удалась
        return EXIT_FAILURE; // Завершаем программу с кодом ошибки
    }

    sf::Texture mainPanelTexture; // Создаем текстуру для поля
    mainPanelTexture.loadFromImage(mainPanelImage); // Загружаем изображение в текстуру

    sf::Sprite mainPanelSprite; // Создаем спрайт для левой боковой панели
    mainPanelSprite.setTexture(mainPanelTexture); // Устанавливаем текстуру для спрайта
    mainPanelSprite.setTextureRect(sf::IntRect(0, 0, 636, 636)); // Устанавливаем область текстуры, которая будет отображаться (часть панели)
    mainPanelSprite.setPosition(365, 67); // Устанавливаем позицию спрайта панели

    sf::Texture clothesTexture;
    if (!clothesTexture.loadFromFile("pictures/clothes.png")) {
        std::cerr << "Error loading clothes.png" << std::endl;
        return EXIT_FAILURE;
    }

    std::random_device rd; // Создаем объект для получения случайных чисел из аппаратного источника (если доступен)
    std::mt19937 gen(rd()); // Создаем генератор случайных чисел Mersenne Twister, инициализируем его с помощью rd
    std::uniform_int_distribution<> distrib(1, 6); // Создаем распределение случайных чисел в диапазоне от 1 до 6 (включительно)

    // Создаем новую tileMap
    std::vector<std::vector<Tile>> tiles(HEIGHT_MAP, std::vector<Tile>(WIDTH_MAP));
    std::vector<std::vector<int>> tileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP));

    //Заполняем углы значением 9
    tileMap[0][0] = 9;          // Левый верхний угол
    tileMap[0][1] = 9;
    tileMap[1][0] = 9;
    tileMap[0][WIDTH_MAP - 1] = 9; // Правый верхний угол
    tileMap[1][WIDTH_MAP - 1] = 9; // Правый верхний угол
    tileMap[0][WIDTH_MAP - 2] = 9; // Правый верхний угол
    tileMap[HEIGHT_MAP - 1][0] = 9;    // Левый нижний угол
    tileMap[HEIGHT_MAP - 1][1] = 9;
    tileMap[HEIGHT_MAP - 1][WIDTH_MAP - 2] = 9;// Правый нижний угол
    tileMap[HEIGHT_MAP - 1][WIDTH_MAP - 1] = 9;   // Правый нижний угол
    tileMap[HEIGHT_MAP - 2][0] = 9;
    tileMap[HEIGHT_MAP - 2][WIDTH_MAP - 1] = 9;// Правый нижний угол

    // Заполняем оставшиеся ячейки случайными числами от 1 до 6
    for (int y = 0; y < HEIGHT_MAP; ++y) {
        for (int x = 0; x < WIDTH_MAP; ++x) {
            if (tileMap[y][x] != 9) {
                tileMap[y][x] = distrib(gen);

                // Всегда устанавливаем текстуру и обновляем спрайт, даже для угловых элементов
                tiles[y][x].sprite.setTexture(clothesTexture);
                tiles[y][x].value = tileMap[y][x];
                tiles[y][x].currentPos = sf::Vector2f(397 + x * squareSize, 99 + y * squareSize);
                tiles[y][x].targetPos = tiles[y][x].currentPos;
                tiles[y][x].updateSprite(clothesTexture); // Обновляем спрайт для всех тайлов
            }
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

    // После создания tileMap и tiles
    MatchChecker matchChecker(tileMap, tiles, clothesTexture);
    matchChecker.start();

    //sf::Time;
    sf::Clock clock;

    bool isAnimating = false; // Флаг, указывающий, выполняется ли анимация
    bool needsCheckMatches = false;
    bool dragging = false;
    int selectedX = -1, selectedY = -1;
    float animationProgress = 0.0f; // Текущий прогресс анимации (от 0.0 до 1.0)

    sf::Vector2f animationStartPos, animationEndPos; // Начальная и конечная позиции для анимации

    sf::RectangleShape mainRect; // Создаем прямоугольник для основной границы сетки

    sf::RectangleShape corner; // Создаем прямоугольник для углов сетки (для закругления)

    sf::RectangleShape line; // Создаем прямоугольник для линий сетки

    sf::Color translucentColor(0, 0, 0, 0); // Создаем полупрозрачный синий цвет (RGBA: Red=0, Green=0, Blue=255, Alpha=64)
    mainRect.setFillColor(translucentColor); // Устанавливаем полупрозрачный цвет заливки для основного прямоугольника сетки

    sf::Color translucentOutlineColor(0, 0, 0, 0); // Создаем полупрозрачный синий цвет для обводки (более плотный, чем заливка)
    mainRect.setOutlineColor(translucentOutlineColor); // Устанавливаем полупрозрачный цвет обводки для основного прямоугольника сетки

    sf::RectangleShape selectedSquare; // Создаем прямоугольник для отображения выбранной ячейки
    sf::Color yellowTransparent(sf::Color::Yellow); // Создаем цвет, начинающийся с желтого
    yellowTransparent.a = 0; // Устанавливаем альфа-канал (прозрачность) желтого цвета в 0 (полностью прозрачный) - изначально прозрачный
    selectedSquare.setFillColor(yellowTransparent); // Устанавливаем цвет заливки для selectedSquare как желтый прозрачный
    selectedSquare.setSize(sf::Vector2f(squareSize, squareSize)); // Устанавливаем размер selectedSquare в соответствии с размером ячейки

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

        if (needsCheckMatches) 
        {
            matchChecker.requestUpdate();
            needsCheckMatches = false;
        }

        if (matchChecker.hasMatches()) 
        {
            auto matches = matchChecker.getMatches();
            removeMatches(tileMap, tiles, clothesTexture, matches, matchChecker);
            needsCheckMatches = true; // Проверить снова после изменений
        }

        while (window.pollEvent(event)) // Цикл обработки событий (закрытия окна, нажатия клавиш, мыши и т.д.)
        {
            findAndReplaceMatches(tileMap, tiles, clothesTexture, matchChecker); // Вызываем функцию для поиска и замены совпадений в tileMap (логика игры). ВЫЗЫВАЕТСЯ ОЧЕНЬ ЧАСТО, возможно, нужно переместить

            if (event.type == sf::Event::Closed) // Если событие - закрытие окна
            {
                window.close(); // Закрываем окно
            }

            // Обработка нажатия кнопки мыши
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                int mouseX = event.mouseButton.x;
                int mouseY = event.mouseButton.y;

                bool foundSelected = false; // Флаг, показывающий, что тайл выбран

                for (int i = 0; i < HEIGHT_MAP; ++i)
                {
                    for (int j = 0; j < WIDTH_MAP; ++j)
                    {
                        if (isSquareSelected(j, i, mouseX, mouseY, squareSize, 397, 99))
                        {
                            selectedX = j;
                            selectedY = i;
                            dragging = true;

                            // Увеличение масштаба выбранного спрайта
                            tiles[i][j].targetScale = 1.2f; // Немного увеличиваем
                            tiles[i][j].scaling = true; //включаем флаг

                            foundSelected = true;
                            break;  // Как только нашли тайл, выходим из внутреннего цикла
                        }
                    }
                    if (foundSelected) break; // Если нашли тайл, выходим и из внешнего цикла
                }
            }

            // Обработка отпускания кнопки мыши (завершение перетаскивания)
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging)
            {
                int mouseX = event.mouseButton.x;
                int mouseY = event.mouseButton.y;

                int targetX = (mouseX - 397) / squareSize;
                int targetY = (mouseY - 99) / squareSize;

                // Проверка, что цель находится в пределах карты
                if (targetX >= 0 && targetX < WIDTH_MAP && targetY >= 0 && targetY < HEIGHT_MAP)
                {
                    // Проверка, что перемещение происходит только на одну клетку
                    if ((abs(targetX - selectedX) + abs(targetY - selectedY)) == 1)
                    {
                        // Обмен значений в tileMap и tiles
                        std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]);
                        std::swap(tiles[selectedY][selectedX], tiles[targetY][targetX]);

                        // Обновление позиции для анимации перемещения (ВАЖНО!)
                        sf::Vector2f tempTarget = tiles[selectedY][selectedX].targetPos;
                        tiles[selectedY][selectedX].targetPos = tiles[targetY][targetX].targetPos;
                        tiles[targetY][targetX].targetPos = tempTarget;

                        tiles[selectedY][selectedX].moving = true;
                        tiles[targetY][targetX].moving = true;

                        needsCheckMatches = true;
                    }
                }

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

                // Возвращение масштаба спрайта к исходному состоянию
                tiles[selectedY][selectedX].targetScale = 1.0f;
                tiles[selectedY][selectedX].scaling = true;

                dragging = false; // Сбрасываем флаг перетаскивания
                selectedX = -1;
                selectedY = -1;

            }
        }


        // Обновление анимаций
        isAnimating = false;
        sf::Time deltaTime = clock.restart();

        for (auto& row : tiles)
        {
            for (auto& tile : row)
            {
                if (tile.moving)
                {
                    // Анимация перемещения
                    sf::Vector2f delta = tile.targetPos - tile.currentPos;
                    tile.currentPos += delta * (animationSpeed * deltaTime.asSeconds() * 60); 

                    // Если тайл достиг целевой позиции, завершаем анимацию
                    if (std::abs(delta.x) < 1.0f && std::abs(delta.y) < 1.0f)
                    {
                        tile.currentPos = tile.targetPos;
                        tile.moving = false;
                    }
                    isAnimating = true;
                }

                if (tile.removing)
                {
                    // Анимация удаления (уменьшение масштаба и прозрачности)
                    tile.scale -= removeAnimationSpeed * deltaTime.asSeconds() * 60;
                    tile.alpha = std::max(0.0f, tile.alpha - 255.0f * removeAnimationSpeed * deltaTime.asSeconds() * 30);

                    // Если анимация завершена, переключаемся на анимацию появления
                    if (tile.scale <= 0.0f)
                    {
                        tile.removing = false;
                        tile.appearing = true;
                        tile.scale = 0.0f;
                        tile.alpha = 0.0f;
                        tile.updateSprite(clothesTexture);// Передаем текстуру одежды
                    }
                    isAnimating = true;
                }

                if (tile.appearing)
                {
                    // Анимация появления (увеличение масштаба и прозрачности)
                    tile.scale += removeAnimationSpeed * deltaTime.asSeconds() * 60;
                    tile.alpha = std::min(255.0f, tile.alpha + 255.0f * removeAnimationSpeed * deltaTime.asSeconds() * 60);

                    // Если анимация завершена, сбрасываем флаг
                    if (tile.scale >= 1.0f)
                    {
                        tile.scale = 1.0f;
                        tile.alpha = 255.0f;
                        tile.appearing = false;
                    }
                    tile.updateSprite(clothesTexture); // Передаем текстуру одежды
                    isAnimating = true;
                }
            }
        }

        if (!isAnimating && needsCheckMatches) {
    if (findAndReplaceMatches(tileMap, tiles, clothesTexture, matchChecker)) {
        needsCheckMatches = true;
    }
    else {
        needsCheckMatches = false;
    }
}

        window.clear(); // Очищаем окно (заливаем черным цветом по умолчанию)
        window.draw(sprite); // Рисуем спрайт фона

        window.draw(mainPanelSprite); // Рисуем основной прямоугольник сетки

        window.draw(leftSidePanelSprite); // Рисуем спрайт левой панели

        window.draw(selectedSquare); // Рисуем квадратик выделения выбранной ячейки

        // Отрисовка тайлов
        for (const auto& row : tiles)
        {
            for (const auto& tile : row)
            {
                sf::Sprite s = tile.sprite;
                s.setPosition(tile.currentPos);
                s.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(tile.alpha)));
                s.setScale(tile.scale, tile.scale);
                window.draw(s);
            }
        }

        window.display(); // Отображаем содержимое окна
    }

    matchChecker.stop();

}