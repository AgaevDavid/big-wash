#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

struct Tile {
    sf::Sprite sprite;
    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    float scale = 1.0f;
    float alpha = 255.0f;
    bool moving = false;
    bool removing = false;
    bool appearing = false;
    int value = 0;
};

void updateTileSprite(Tile& tile, const sf::Texture& clothesTexture)
{
    // Проверяем, что значение тайла находится в допустимом диапазоне (от 1 до 6)
    if (tile.value >= 1 && tile.value <= 6)
    {
        // Устанавливаем текстуру для спрайта, если она еще не установлена
        if (tile.sprite.getTexture() == nullptr)
        {
            tile.sprite.setTexture(clothesTexture);
        }

        // Вычисляем индекс элемента одежды (учитываем, что значения в tileMap от 1 до 6)
        int tileIndex = tile.value - 1;

        // Устанавливаем область текстуры для спрайта одежды (выбираем нужный элемент)
        tile.sprite.setTextureRect(sf::IntRect(tileIndex * 64, 0, 64, 64));
    }
}

// Функция для поиска и замены совпадений
void findAndReplaceMatches(std::vector<std::vector<int>>& tileMap, std::vector<std::vector<Tile>>& tiles, sf::Texture& clothesTexture)
{
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
            else
            {
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
            if (tileMap[y][x] == 9)
            {
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
                    tileMap[y][x] = rand() % 6 + 1;
                    tiles[y][x].value = tileMap[y][x];
                    tiles[y][x].appearing = true; // Запуск анимации появления
                    updateTileSprite(tiles[y][x], clothesTexture); // Передаем текстуру одежды
                }
            }
        }
    }
}

static bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int startX, int startY) //проверяет в какой квадрат указывает мышь
{
    return mouseX >= startX + x * squareSize && mouseX < startX + (x + 1) * squareSize &&
        mouseY >= startY + y * squareSize && mouseY < startY + (y + 1) * squareSize;
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

    //sf::Image clothes; // Создаем изображение для элементов одежды
    //clothes.loadFromFile("pictures/clothes.png"); // Загружаем изображение одежды из файла

    //sf::Texture clothes1; // Создаем текстуру для элементов одежды
    //clothes1.loadFromImage(clothes); // Загружаем изображение в текстуру
    //if (!clothes.loadFromFile("pictures/clothes.png")) // Повторно загружаем изображение и проверяем на ошибку (дублирование проверки)
    //{
    //    return EXIT_FAILURE; // Завершаем программу с кодом ошибки, если загрузка не удалась
    //}

    //sf::Sprite s_clothes; // Создаем спрайт для элементов одежды
    //s_clothes.setTexture(clothes1); // Устанавливаем текстуру для спрайта одежды

    sf::Texture clothesTexture;
    if (!clothesTexture.loadFromFile("pictures/clothes.png")) {
        std::cerr << "Error loading clothes.png" << std::endl;
        return EXIT_FAILURE;
    }

    const int startX = 397; // Определяем константу для начальной X-координаты сетки на экране
    const int startY = 99;  // Определяем константу для начальной Y-координаты сетки на экране

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
                tiles[y][x].sprite.setTexture(clothesTexture);
                tiles[y][x].value = tileMap[y][x];
                tiles[y][x].currentPos = sf::Vector2f(397 + x * squareSize, 99 + y * squareSize);
                tiles[y][x].targetPos = tiles[y][x].currentPos;
                updateTileSprite(tiles[y][x], clothesTexture); // Передаем текстуру одежд
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
            findAndReplaceMatches(tileMap, tiles, clothesTexture); // Вызываем функцию для поиска и замены совпадений в tileMap (логика игры). ВЫЗЫВАЕТСЯ ОЧЕНЬ ЧАСТО, возможно, нужно переместить

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
                        if (isSquareSelected(j, i, mouseX, mouseY, squareSize, 397, 99)) 
                        {
                            selectedX = j;
                            selectedY = i;
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

                int targetX = (mouseX - 397) / squareSize;
                int targetY = (mouseY - 99) / squareSize;

                if (targetX >= 0 && targetX < WIDTH_MAP && targetY >= 0 && targetY < HEIGHT_MAP &&
                    (abs(targetX - selectedX) + abs(targetY - selectedY)) == 1) 
                {
                    std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]);
                    std::swap(tiles[selectedY][selectedX], tiles[targetY][targetX]);
                    needsCheckMatches = true;
                }

                //dragging = false; // Сбрасываем флаг перетаскивания
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
            for (auto& tiles : row)
            {
                if (tiles.moving)
                {
                    // Анимация перемещения
                    sf::Vector2f delta = tiles.targetPos - tiles.currentPos;
                    tiles.currentPos += delta * (animationSpeed * deltaTime.asSeconds() * 60);

                    // Если тайл достиг целевой позиции, завершаем анимацию
                    if (std::abs(delta.x) < 1.0f && std::abs(delta.y) < 1.0f) 
                    {
                        tiles.currentPos = tiles.targetPos;
                        tiles.moving = false;
                    }
                    isAnimating = true;
                }

                if (tiles.removing) 
                {
                    // Анимация удаления (уменьшение масштаба и прозрачности)
                    tiles.scale -= removeAnimationSpeed * deltaTime.asSeconds() * 60;
                    tiles.alpha = std::max(0.0f, tiles.alpha - 255.0f * removeAnimationSpeed * deltaTime.asSeconds() * 60);

                    // Если анимация завершена, переключаемся на анимацию появления
                    if (tiles.scale <= 0.0f) 
                    {
                        tiles.removing = false;
                        tiles.appearing = true;
                        tiles.scale = 0.0f;
                        tiles.alpha = 0.0f;
                        updateTileSprite(tiles, clothesTexture);// Передаем текстуру одежды
                    }
                    isAnimating = true;
                }

                if (tiles.appearing) 
                {
                    // Анимация появления (увеличение масштаба и прозрачности)
                    tiles.scale += removeAnimationSpeed * deltaTime.asSeconds() * 60;
                    tiles.alpha = std::min(255.0f, tiles.alpha + 255.0f * removeAnimationSpeed * deltaTime.asSeconds() * 60);

                    // Если анимация завершена, сбрасываем флаг
                    if (tiles.scale >= 1.0f) 
                    {
                        tiles.scale = 1.0f;
                        tiles.alpha = 255.0f;
                        tiles.appearing = false;
                    }
                    updateTileSprite(tiles, clothesTexture); // Передаем текстуру одежды
                    isAnimating = true;
                }
            }
        }

        if (!isAnimating && needsCheckMatches)
        {
            findAndReplaceMatches(tileMap, tiles, clothesTexture);
            needsCheckMatches = false;
        }

        window.clear(); // Очищаем окно (заливаем черным цветом по умолчанию)
        window.draw(sprite); // Рисуем спрайт фона

        window.draw(mainPanelSprite); // Рисуем основной прямоугольник сетки

        window.draw(leftSidePanelSprite); // Рисуем спрайт левой панели

        window.draw(selectedSquare); // Рисуем квадратик выделения выбранной ячейки

        //// Рисуем элементы одежды на игровом поле
        //for (int i = 0; i < HEIGHT_MAP; ++i)
        //{
        //    for (int j = 0; j < WIDTH_MAP; ++j)
        //    {
        //        //tiles[i][j].sprite.setTexture(clothes1);
        //        tiles[i][j].currentPos = sf::Vector2f(startX + j * squareSize, startY + i * squareSize);
        //        tiles[i][j].targetPos = tiles[i][j].currentPos;
        //        tiles[i][j].value = tileMap[i][j];
        //        int tileIndex = tileMap[i][j] - 1; // Получаем индекс элемента одежды (учитываем, что значения в tileMap от 1 до 6)
        //        if (tileIndex >= 0 && tileIndex < 6) // Проверяем, что индекс находится в допустимом диапазоне
        //        {
        //            s_clothes.setTextureRect(sf::IntRect(tileIndex * 64, 0, 64, 64)); // Устанавливаем область текстуры для спрайта одежды (выбираем нужный элемент)
        //            s_clothes.setPosition(startX + j * 84, startY + i * 84); // Устанавливаем позицию спрайта одежды
        //            window.draw(s_clothes); // Рисуем спрайт одежды
        //        }
        //    }
        //}

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

        // Если выполняется перетаскивание, рисуем квадратик выделения рядом с мышью
        if (dragging && selectedX != -1 && selectedY != -1)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window); // Получаем позицию мыши относительно окна
            selectedSquare.setPosition((float)(mousePos.x - squareSize / 2), (float)(mousePos.y - squareSize / 2)); // Устанавливаем позицию квадратика выделения (центр квадратика под курсором)
            window.draw(selectedSquare); // Рисуем квадратик выделения
        }

        window.display(); // Отображаем содержимое окна
    }
}

//некорректно отображается тайл отностительно порядкового номера 
//некорректна анимация движения тайла (она почти совсем пропала) частично проблема выходит из предыдущей
//осмотр функции findAndReplaceMatches (возможно часть проблемы с анимацией имеют произхождение в данной функции)