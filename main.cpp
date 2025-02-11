#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>

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
        else if (value == 9)
        {
            // Угловые элементы: скрываем или устанавливаем другую текстуру
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

bool findAndReplaceMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, sf::Texture& clothesTexture)
{
    int height = tileMap.size();
    if (height == 0) return false;
    int width = tileMap[0].size();
    if (width == 0) return false;

    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));
    bool matchesFound = false;

    // Поиск горизонтальных совпадений
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
            else
            {
                if (matchLength >= 3)
                {
                    for (int k = x - matchLength; k < x; ++k)
                    {
                        toRemove[y][k] = true;
                        matchesFound = true;
                    }
                }
                start = x;
                matchLength = 1;
            }
        }
    }

    // Поиск вертикальных совпадений
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
                        matchesFound = true;
                    }
                }
                start = y;
                matchLength = 1;
            }
        }
    }

    if (!matchesFound)
    {
        return false;
    }

    // Создаем генератор случайных чисел и распределение
    std::random_device rd; // Используем для получения случайного seed
    std::mt19937 gen(rd()); // Mersenne Twister engine
    std::uniform_int_distribution<> distrib(1, 6); // Распределение от 1 до 6

    for (int x = 0; x < width; ++x)
    {
        std::vector<int> splitIndices;
        splitIndices.push_back(-1);
        for (int y = 0; y < height; ++y)
        {
            if (tileMap[y][x] == 9)
            {
                splitIndices.push_back(y);
            }
        }
        splitIndices.push_back(height);

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
            }

            int removedCount = segLen - survivingElements.size();
            int writeY = segStart;

            // Заполняем удаленные позиции нулями
            for (int j = 0; j < removedCount; ++j, ++writeY)
            {
                tileMap[writeY][x] = 0;
                tiles[writeY][x].value = 0;
                tiles[writeY][x].removing = true; // Запуск анимации удаления
            }

            // Записываем выжившие элементы
            for (int val : survivingElements)
            {
                if (writeY >= segEnd) break;
                tileMap[writeY][x] = val;
                tiles[writeY][x].value = val;
                tiles[writeY][x].targetPos = sf::Vector2f(
                    tiles[writeY][x].currentPos.x,
                    tiles[writeY][x].currentPos.y
                );
                tiles[writeY][x].moving = true; // Запуск анимации перемещения
                writeY++;
            }

            // Заменяем нули на случайные числа 1-6
            for (int y = segStart; y < segEnd; ++y)
            {
                if (tileMap[y][x] == 0)
                {
                    tileMap[y][x] = distrib(gen); // Используем distrib(gen) для генерации случайных чисел
                    tiles[y][x].value = tileMap[y][x];
                    tiles[y][x].appearing = true; // Запуск анимации появления
                    tiles[y][x].updateSprite(clothesTexture);
                }
            }
        }
    }

    return true;
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

    const int HEIGHT_MAP = 7; // Определяем константу для высоты игрового поля (7 клеток)
    const int WIDTH_MAP = 7;  // Определяем константу для ширины игрового поля (7 клеток)
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

    // Заполняем углы значением 9
    tileMap[0][0] = 9;          // Левый верхний угол
    tileMap[0][1] = 9;
    tileMap[0][WIDTH_MAP - 2] = 9; // Правый верхний угол
    tileMap[0][WIDTH_MAP - 1] = 9; // Правый верхний угол
    tileMap[1][0] = 9;
    tileMap[1][WIDTH_MAP - 1] = 9; // Правый верхний угол
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

    findAndReplaceMatches(tileMap, tiles, clothesTexture);

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

    sf::Time;
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

        while (window.pollEvent(event)) // Цикл обработки событий (закрытия окна, нажатия клавиш, мыши и т.д.)
        {
            //findAndReplaceMatches(tileMap, tiles, clothesTexture); // Вызываем функцию для поиска и замены совпадений в tileMap (логика игры). ВЫЗЫВАЕТСЯ ОЧЕНЬ ЧАСТО, возможно, нужно переместить

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

                // Возвращение масштаба спрайта к исходному состоянию
                tiles[selectedY][selectedX].targetScale = 1.0f;
                tiles[selectedY][selectedX].scaling = true;

                dragging = false; // Сбрасываем флаг перетаскивания
                selectedX = -1;
                selectedY = -1;

                std::cout << "new map:" << std::endl; // Выводим в консоль метку "new map:"
                for (int y = 0; y < HEIGHT_MAP; ++y) // Цикл по строкам игрового поля
                {
                    for (int x = 0; x < WIDTH_MAP; ++x) // Цикл по столбцам игрового поля
                    {
                        std::cout << tileMap[y][x] << " "; // Выводим значение ячейки и пробел
                    }
                    std::cout << std::endl; // Переходим на новую строку после каждой строки
                }
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
                    tile.alpha = std::max(0.0f, tile.alpha - 255.0f * removeAnimationSpeed * deltaTime.asSeconds() * 60);

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

        if (!isAnimating && needsCheckMatches)
        {
            if (findAndReplaceMatches(tileMap, tiles, clothesTexture)) {
                needsCheckMatches = true; // Если совпадения были найдены, нужно проверить еще раз
            }
            else {
                needsCheckMatches = false; // Если совпадений не найдено, больше не проверяем
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
}