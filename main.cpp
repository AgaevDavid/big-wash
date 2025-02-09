#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>  // Для stringstream
#include <iomanip>  // Для setw и setfill

static bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int startX, int startY) //проверяет в какой квадрат указывает мышь
{
    return mouseX >= startX + x * squareSize && mouseX < startX + (x + 1) * squareSize &&
        mouseY >= startY + y * squareSize && mouseY < startY + (y + 1) * squareSize;
}

void findAndReplaceMatches(std::vector<std::vector<int>>& tileMap) {
    srand(time(0)); // Инициализация генератора случайных чисел

    int height = tileMap.size();
    if (height == 0) return;
    int width = tileMap[0].size();
    if (width == 0) return;

    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));

    // Поиск горизонтальных совпадений (игнорируя 0 и 9)
    for (int y = 0; y < height; ++y) {
        int start = 0;
        int matchLength = 1;
        for (int x = 1; x <= width; ++x) {
            if (x < width && tileMap[y][x] == tileMap[y][x - 1] && tileMap[y][x] != 0 && tileMap[y][x] != 9) {
                matchLength++;
            }
            else {
                if (matchLength >= 3) {
                    for (int k = x - matchLength; k < x; ++k) {
                        toRemove[y][k] = true;
                    }
                }
                start = x;
                matchLength = 1;
            }
        }
    }

    // Поиск вертикальных совпадений (игнорируя 0 и 9)
    for (int x = 0; x < width; ++x) {
        int start = 0;
        int matchLength = 1;
        for (int y = 1; y <= height; ++y) {
            if (y < height && tileMap[y][x] == tileMap[y - 1][x] && tileMap[y][x] != 0 && tileMap[y][x] != 9) {
                matchLength++;
            }
            else {
                if (matchLength >= 3) {
                    for (int k = y - matchLength; k < y; ++k) {
                        toRemove[k][x] = true;
                    }
                }
                start = y;
                matchLength = 1;
            }
        }
    }

    // Обновление столбцов с учетом 9 и замена нулей
    for (int x = 0; x < width; ++x) {
        std::vector<int> splitIndices;
        splitIndices.push_back(-1); // Начальная граница
        for (int y = 0; y < height; ++y) {
            if (tileMap[y][x] == 9) {
                splitIndices.push_back(y);
            }
        }
        splitIndices.push_back(height); // Конечная граница

        for (int i = 0; i < splitIndices.size() - 1; ++i) {
            int segStart = splitIndices[i] + 1;
            int segEnd = splitIndices[i + 1];
            int segLen = segEnd - segStart;

            if (segLen <= 0) continue;

            std::vector<int> survivingElements;
            for (int y = segStart; y < segEnd; ++y) {
                if (!toRemove[y][x]) {
                    survivingElements.push_back(tileMap[y][x]);
                }
            }

            int removedCount = segLen - survivingElements.size();
            int writeY = segStart;

            // Заполняем удаленные позиции нулями
            for (int j = 0; j < removedCount; ++j, ++writeY) {
                tileMap[writeY][x] = 0;
            }

            // Записываем выжившие элементы
            for (int val : survivingElements) {
                if (writeY >= segEnd) break;
                tileMap[writeY][x] = val;
                ++writeY;
            }

            // Заменяем нули на случайные числа 1-6
            for (int y = segStart; y < segEnd; ++y) {
                if (tileMap[y][x] == 0) {
                    tileMap[y][x] = rand() % 6 + 1;
                }
            }
        }
    }
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

    sf::RenderWindow window(sf::VideoMode(1366, 770), "Big wash"); // Создаем окно SFML с разрешением 1366x770 и заголовком "Big wash"

    sf::Texture background; // Создаем текстуру для фона
    background.loadFromFile("pictures/background.png"); // Загружаем изображение фона из файла "pictures/background.png"

    sf::Sprite sprite(background); // Создаем спрайт (графический объект) для фона, используя загруженную текстуру
    sprite.setTextureRect(sf::IntRect(0, 0, 1366, 770)); // Устанавливаем область текстуры, которая будет отображаться (весь фон)
    sprite.setPosition(0, 0); // Устанавливаем позицию спрайта фона в левый верхний угол окна

    //добавляем панель с левой стороны экрана
    sf::Image leftSidePanelImage; // Создаем изображение для левой боковой панели
    if (!leftSidePanelImage.loadFromFile("pictures/panel_L_arrows.png")) { // Загружаем изображение панели из файла, проверяем на ошибку
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

    sf::Image clothes; // Создаем изображение для элементов одежды
    clothes.loadFromFile("pictures/clothes.png"); // Загружаем изображение одежды из файла
    sf::Texture clothes1; // Создаем текстуру для элементов одежды
    clothes1.loadFromImage(clothes); // Загружаем изображение в текстуру
    if (!clothes.loadFromFile("pictures/clothes.png")) // Повторно загружаем изображение и проверяем на ошибку (дублирование проверки)
    {
        return EXIT_FAILURE; // Завершаем программу с кодом ошибки, если загрузка не удалась
    }

    sf::Sprite s_clothes; // Создаем спрайт для элементов одежды
    s_clothes.setTexture(clothes1); // Устанавливаем текстуру для спрайта одежды

    const int startX = 397; // Определяем константу для начальной X-координаты сетки на экране
    const int startY = 99;  // Определяем константу для начальной Y-координаты сетки на экране

    std::random_device rd; // Создаем объект для получения случайных чисел из аппаратного источника (если доступен)
    std::mt19937 gen(rd()); // Создаем генератор случайных чисел Mersenne Twister, инициализируем его с помощью rd
    std::uniform_int_distribution<> distrib(1, 6); // Создаем распределение случайных чисел в диапазоне от 1 до 6 (включительно)

    std::vector< std::vector<int>> tileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP)); // Создаем новую tileMap

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
            if (tileMap[y][x] != 9) {  // Если ячейка не является углом (не равна 9)
                tileMap[y][x] = distrib(gen); // Заполняем случайным числом
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

    bool animating = false; // Флаг, указывающий, выполняется ли анимация
    float animationProgress = 0.0f; // Текущий прогресс анимации (от 0.0 до 1.0)
    int animationTargetX = -1, animationTargetY = -1; // Координаты ячейки, к которой происходит анимация
    sf::Vector2f animationStartPos, animationEndPos; // Начальная и конечная позиции для анимации

    const int gridSize = 606; // Размер сетки игрового поля (в пикселях)
    const int squareSize = gridSize / 7; // Размер одной ячейки (квадрата) в сетке (в пикселях)
    const float thinLineWidth = 0.f; // Толщина тонких линий сетки
    const float thickLineWidth = 0.f; // Толщина толстых линий сетки
    const float cornerRadius = 0.f; // Радиус углов сетки

    const float gridX = (1366 - gridSize) / 2.f; // X-координата левого верхнего угла сетки, чтобы она была по центру по горизонтали
    const float gridY = (770 - gridSize) / 2.f;  // Y-координата левого верхнего угла сетки, чтобы она была по центру по вертикали

    sf::RectangleShape mainRect; // Создаем прямоугольник для основной границы сетки
    mainRect.setSize({ (float)gridSize, (float)gridSize }); // Устанавливаем размер прямоугольника равным размеру сетки
    mainRect.setFillColor(sf::Color::Transparent); // Устанавливаем прозрачный цвет заливки (чтобы видеть элементы под сеткой)
    mainRect.setOutlineColor(sf::Color::Black); // Устанавливаем черный цвет обводки
    mainRect.setOutlineThickness(thickLineWidth); // Устанавливаем толщину обводки
    mainRect.setPosition(gridX, gridY); // Устанавливаем позицию прямоугольника

    sf::RectangleShape corner; // Создаем прямоугольник для углов сетки (для закругления)
    corner.setFillColor(sf::Color::Black); // Устанавливаем черный цвет заливки
    corner.setSize({ cornerRadius * 10, cornerRadius * 10 }); // Устанавливаем размер углового прямоугольника

    sf::RectangleShape line; // Создаем прямоугольник для линий сетки
    line.setFillColor(sf::Color::Black); // Устанавливаем черный цвет заливки
    line.setOutlineThickness(thinLineWidth); // Устанавливаем толщину обводки

    sf::Color translucentColor(0, 0, 0, 0); // Создаем полупрозрачный синий цвет (RGBA: Red=0, Green=0, Blue=255, Alpha=64)
    mainRect.setFillColor(translucentColor); // Устанавливаем полупрозрачный цвет заливки для основного прямоугольника сетки

    sf::Color translucentOutlineColor(0, 0, 0, 0); // Создаем полупрозрачный синий цвет для обводки (более плотный, чем заливка)
    mainRect.setOutlineColor(translucentOutlineColor); // Устанавливаем полупрозрачный цвет обводки для основного прямоугольника сетки

    sf::RectangleShape selectedSquare; // Создаем прямоугольник для отображения выбранной ячейки
    sf::Color yellowTransparent(sf::Color::Yellow); // Создаем цвет, начинающийся с желтого
    yellowTransparent.a = 0; // Устанавливаем альфа-канал (прозрачность) желтого цвета в 0 (полностью прозрачный) - изначально прозрачный
    selectedSquare.setFillColor(yellowTransparent); // Устанавливаем цвет заливки для selectedSquare как желтый прозрачный
    selectedSquare.setSize(sf::Vector2f(squareSize, squareSize)); // Устанавливаем размер selectedSquare в соответствии с размером ячейки

    int selectedX = -1, selectedY = -1; // Инициализируем координаты выбранной ячейки как -1 (нет выбранной ячейки)
    bool dragging = false;   // Флаг, указывающий, выполняется ли перетаскивание

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
            findAndReplaceMatches(tileMap); // Вызываем функцию для поиска и замены совпадений в tileMap (логика игры). ВЫЗЫВАЕТСЯ ОЧЕНЬ ЧАСТО, возможно, нужно переместить

            if (event.type == sf::Event::Closed) // Если событие - закрытие окна
            {
                window.close(); // Закрываем окно
            }

            // Обработка нажатия кнопки мыши
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                int mouseX = event.mouseButton.x; // Получаем X-координату мыши
                int mouseY = event.mouseButton.y; // Получаем Y-координату мыши

                // Ищем, была ли нажата какая-либо ячейка на игровом поле
                for (int i = 0; i < HEIGHT_MAP; ++i) // Цикл по строкам игрового поля
                {
                    for (int j = 0; j < WIDTH_MAP; ++j) // Цикл по столбцам игрового поля
                    {
                        if (isSquareSelected(j, i, mouseX, mouseY, squareSize, startX, startY)) // Проверяем, была ли нажата ячейка с координатами (j, i)
                        {
                            selectedX = j; // Запоминаем X-координату выбранной ячейки
                            selectedY = i; // Запоминаем Y-координату выбранной ячейки
                            selectedSquare.setPosition(startX + j * squareSize, startY + i * squareSize); // Устанавливаем позицию selectedSquare (квадратика выделения)
                            dragging = true; // Устанавливаем флаг перетаскивания

                            findAndReplaceMatches(tileMap); // Вызываем функцию для поиска и замены совпадений (тоже вызывается слишком часто)

                            break; // Выходим из внутреннего цикла, т.к. ячейка уже найдена
                        }
                    }
                }
            }

            // Обработка отпускания кнопки мыши (завершение перетаскивания)
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging)
            {
                findAndReplaceMatches(tileMap); // Вызываем функцию для поиска и замены совпадений (тоже вызывается слишком часто)

                int mouseX = event.mouseButton.x; // Получаем X-координату мыши
                int mouseY = event.mouseButton.y; // Получаем Y-координату мыши

                int targetX = (mouseX - startX) / squareSize; // Вычисляем X-координату целевой ячейки на игровом поле
                int targetY = (mouseY - startY) / squareSize; // Вычисляем Y-координату целевой ячейки на игровом поле

                // Проверяем, что целевая ячейка находится в пределах игрового поля и является соседней с выбранной
                if (targetX >= 0 && targetX < WIDTH_MAP && targetY >= 0 && targetY < HEIGHT_MAP &&
                    (abs(targetX - selectedX) + abs(targetY - selectedY)) == 1) // Проверяем соседство (расстояние по X + расстояние по Y = 1)
                {
                    std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]); // Меняем местами элементы в tileMap (свапаем ячейки)
                    selectedX = -1; // Сбрасываем X-координату выбранной ячейки
                    selectedY = -1; // Сбрасываем Y-координату выбранной ячейки

                    findAndReplaceMatches(tileMap); // Вызываем функцию для поиска и замены совпадений(тоже вызывается слишком часто)
                }

                dragging = false; // Сбрасываем флаг перетаскивания

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

        window.clear(); // Очищаем окно (заливаем черным цветом по умолчанию)
        window.draw(sprite); // Рисуем спрайт фона

        window.draw(mainPanelSprite); // Рисуем основной прямоугольник сетки

        window.draw(mainRect);
       
        line.setSize({ thinLineWidth, (float)gridSize }); // Устанавливаем размер линии для вертикальных линий

        //Рисуем вертикальные линии сетки
        for (int i = 1; i < 7; ++i)
        {
            line.setPosition(gridX + (float)(i * squareSize), gridY); // Устанавливаем позицию линии
            window.draw(line); // Рисуем линию
        }

        line.setSize({ (float)gridSize, thinLineWidth }); // Устанавливаем размер линии для горизонтальных линий

        // Рисуем горизонтальные линии сетки
        for (int i = 1; i < 7; ++i)
        {
            line.setPosition(gridX, gridY + (float)(i * squareSize)); // Устанавливаем позицию линии
            window.draw(line); // Рисуем линию
        }

        window.draw(leftSidePanelSprite); // Рисуем спрайт левой панели

        window.draw(selectedSquare); // Рисуем квадратик выделения выбранной ячейки

        // Рисуем элементы одежды на игровом поле
        for (int i = 0; i < HEIGHT_MAP; ++i)
        {
            for (int j = 0; j < WIDTH_MAP; ++j)
            {
                int tileIndex = tileMap[i][j] - 1; // Получаем индекс элемента одежды (учитываем, что значения в tileMap от 1 до 6)
                if (tileIndex >= 0 && tileIndex < 6) // Проверяем, что индекс находится в допустимом диапазоне
                {
                    s_clothes.setTextureRect(sf::IntRect(tileIndex * 64, 0, 64, 64)); // Устанавливаем область текстуры для спрайта одежды (выбираем нужный элемент)
                    s_clothes.setPosition(startX + j * 84, startY + i * 84); // Устанавливаем позицию спрайта одежды
                    window.draw(s_clothes); // Рисуем спрайт одежды
                }
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
