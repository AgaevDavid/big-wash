#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <sstream>  // Для stringstream
#include <iomanip>  // Для setw и setfill
#include "removeRowMatches.h"
#include "removeColMatches.h"
#include "rotateForward.h"
#include "rotateBack.h"

static bool isSquareSelected(int x, int y, int mouseX, int mouseY, int squareSize, int startX, int startY) //проверяет в какой квадрат указывает мышь
{
    return mouseX >= startX + x * squareSize && mouseX < startX + (x + 1) * squareSize &&
        mouseY >= startY + y * squareSize && mouseY < startY + (y + 1) * squareSize;
}

void findAndReplaceMatches(std::vector<std::vector<int>>& tileMap) {
    int height = tileMap.size();
    if (height == 0) return;
    int width = tileMap[0].size();
    if (width == 0) return;

    // Матрица для отметки элементов, которые нужно удалить
    std::vector<std::vector<bool>> toRemove(height, std::vector<bool>(width, false));

    // Поиск горизонтальных совпадений
    for (int y = 0; y < height; ++y) {
        int start = 0;
        int matchLength = 1; // Длина текущего совпадения
        for (int x = 1; x <= width; ++x) {
            if (x < width && tileMap[y][x] == tileMap[y][x - 1] && tileMap[y][x] != 0) {
                // Продолжаем совпадение
                matchLength++;
            }
            else {
                // Совпадение закончилось или отличается элемент
                if (matchLength >= 3) {
                    // Отмечаем элементы для удаления
                    for (int k = x - matchLength; k < x; ++k) {
                        toRemove[y][k] = true;
                    }
                }
                // Начинаем новое совпадение, если это не конец строки
                start = x;
                matchLength = 1; // Сбрасываем длину совпадения
            }
        }
    }

    // Поиск вертикальных совпадений
    for (int x = 0; x < width; ++x) {
        int start = 0;
        int matchLength = 1; // Длина текущего совпадения
        for (int y = 1; y <= height; ++y) {
            if (y < height && tileMap[y][x] == tileMap[y - 1][x] && tileMap[y][x] != 0) {
                // Продолжаем совпадение
                matchLength++;
            }
            else {
                // Совпадение закончилось или отличается элемент
                if (matchLength >= 3) {
                    // Отмечаем элементы для удаления
                    for (int k = y - matchLength; k < y; ++k) {
                        toRemove[k][x] = true;
                    }
                }
                // Начинаем новое совпадение, если это не конец столбца
                start = y;
                matchLength = 1; // Сбрасываем длину совпадения
            }
        }
    }

    // Обновление столбцов: удаление отмеченных элементов и сдвиг
    for (int x = 0; x < width; ++x) {
        std::vector<int> newColumn;
        // Собираем элементы, которые не нужно удалять
        for (int y = 0; y < height; ++y) {
            if (!toRemove[y][x]) {
                newColumn.push_back(tileMap[y][x]);
            }
        }
        // Количество удаленных элементов в текущем столбце
        int removedCount = height - newColumn.size();
        // Заполняем верхнюю часть столбца нулями
        for (int y = 0; y < removedCount; ++y) {
            tileMap[y][x] = 0;
        }
        // Заполняем оставшуюся часть столбца сохраненными элементами
        for (int y = 0; y < newColumn.size(); ++y) {
            tileMap[removedCount + y][x] = newColumn[y];
        }
    }
}

int countMoves(const std::vector<std::vector<int>>& before, const std::vector<std::vector<int>>& after) {
    int movesCount = 0;
    int height = before.size();
    int width = (height > 0) ? before[0].size() : 0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (before[y][x] != after[y][x]) { // Если элемент изменился, это ход
                movesCount++;
            }
        }
    }
    return movesCount;
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
    setlocale(LC_ALL, "RUSSIAN");

    const int HEIGHT_MAP = 10;
    const int WIDTH_MAP = 10;

    sf::RenderWindow window(sf::VideoMode(1366, 770), "Big wash");

    sf::Texture background; //добавление фона в окно
    background.loadFromFile("pictures/background.png");

    sf::Sprite sprite(background); 
    sprite.setTextureRect(sf::IntRect(0, 0, 1366, 770)); 
    sprite.setPosition(0, 0);

    //добавляем панель с левой стороны экрана
    sf::Image leftSidePanelImage;
    if (!leftSidePanelImage.loadFromFile("pictures/panel_L_arrows.png")) {
        std::cerr << "Error loading pictures/panel_L_arrows.png" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Texture leftSidePanelTexture;
    leftSidePanelTexture.loadFromImage(leftSidePanelImage);

    sf::Sprite leftSidePanelSprite;
    leftSidePanelSprite.setTexture(leftSidePanelTexture);
    leftSidePanelSprite.setTextureRect(sf::IntRect(0, 0, 175, 521));
    leftSidePanelSprite.setPosition(10, 100);

    sf::Image clothes; //добавление различных элементов одежды в качестве "камней"
    clothes.loadFromFile("pictures/clothes.png");
    sf::Texture clothes1;
    clothes1.loadFromImage(clothes);
    if (!clothes.loadFromFile("pictures/clothes.png"))
    {
        return EXIT_FAILURE;
    }

    sf::Sprite s_clothes;
    s_clothes.setTexture(clothes1);

    const int startX = 363;
    const int startY = 65;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    std::vector < std::vector <int >> TileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP));

    for (int y = 0; y < HEIGHT_MAP; ++y)
    {
        for (int x = 0; x < WIDTH_MAP; ++x)
        {
            TileMap[y][x] = distrib(gen);
        }
    }

    bool animating = false;
    float animationProgress = 0.0f; 
    int animationTargetX = -1, animationTargetY = -1;
    sf::Vector2f animationStartPos, animationEndPos;

    const int gridSize = 640;
    const int squareSize = gridSize / 10;
    const float thinLineWidth = 1.f;
    const float thickLineWidth = 5.f;
    const float cornerRadius = 10.f;

    const float gridX = (1366 - gridSize) / 2.f;
    const float gridY = (770 - gridSize) / 2.f;

    sf::RectangleShape mainRect;
    mainRect.setSize({ (float)gridSize, (float)gridSize });
    mainRect.setFillColor(sf::Color::Transparent);
    mainRect.setOutlineColor(sf::Color::Black);
    mainRect.setOutlineThickness(thickLineWidth);
    mainRect.setPosition(gridX, gridY);

    sf::RectangleShape corner;
    corner.setFillColor(sf::Color::Black);
    corner.setSize({ cornerRadius * 10, cornerRadius * 10 });

    sf::RectangleShape line;
    line.setFillColor(sf::Color::Black);
    line.setOutlineThickness(thinLineWidth);

    sf::Color translucentColor(0, 0, 255, 64);
    mainRect.setFillColor(translucentColor);

    sf::Color translucentOutlineColor(0, 0, 255, 128);
    mainRect.setOutlineColor(translucentOutlineColor);

    sf::Color translucentInLineColor(0, 0, 255, 255);
    line.setFillColor(translucentColor);

    sf::RectangleShape selectedSquare;
    sf::Color yellowTransparent(sf::Color::Yellow); 
    yellowTransparent.a = 0; 
    selectedSquare.setFillColor(yellowTransparent);
    selectedSquare.setSize(sf::Vector2f(squareSize, squareSize));

    int selectedX = -1, selectedY = -1; 
    bool dragging = false;   

    sf::Font font;
    if (!font.loadFromFile("fonts/fredfredburgerheadline.otf")) {
        std::cerr << "Ошибка загрузки шрифта fonts/fredfredburgerheadline.otf" << std::endl;
        return -1;
    }

    // Копируем TileMap до изменений, чтобы можно было посчитать TileMap.
    std::vector<std::vector<int>> beforeTileMap = TileMap; // Важно!

    // Вызываем функцию findAndReplaceMatches для удаления совпадающих элементов
    findAndReplaceMatches(TileMap);

    // Считаем кол-во ходов
    int totalMoves = countMoves(beforeTileMap, TileMap);

    sf::Text text;
    text.setFont(font);
    text.setString(std::to_string(totalMoves)); // Преобразуем число в строку
    text.setCharacterSize(60);
    text.setFillColor(sf::Color::White);
    text.setPosition(90, 523);
    text.setOutlineThickness(3.0f); // Или другое значение толщины
    text.setOutlineColor(hexToColor("#6b46d5"));

    std::vector< std::vector<int>> tileMap(HEIGHT_MAP, std::vector<int>(WIDTH_MAP));
    for (int y = 0; y < HEIGHT_MAP; ++y)
    {
        for (int x = 0; x < WIDTH_MAP; ++x)
        {
            tileMap[y][x] = distrib(gen);
        }
    }

    std::cout << "old map:" << std::endl;
    for (int y = 0; y < HEIGHT_MAP; ++y) {
        for (int x = 0; x < WIDTH_MAP; ++x) {
            std::cout << tileMap[y][x] << " ";
        }
        std::cout << std::endl;
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            findAndReplaceMatches(tileMap);          

            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

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

                            findAndReplaceMatches(tileMap);

                            break;
                        }
                    }
                }
            }

            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging) 
            {
                findAndReplaceMatches(tileMap);
            
                int mouseX = event.mouseButton.x;
                int mouseY = event.mouseButton.y;
                int targetX = (mouseX - startX) / squareSize;
                int targetY = (mouseY - startY) / squareSize;

                if (targetX >= 0 && targetX < WIDTH_MAP && targetY >= 0 && targetY < HEIGHT_MAP &&
                    (abs(targetX - selectedX) + abs(targetY - selectedY)) == 1)
                {
                    std::swap(tileMap[selectedY][selectedX], tileMap[targetY][targetX]);
                    selectedX = -1;
                    selectedY = -1;

                    findAndReplaceMatches(tileMap);
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

        window.clear();
        window.draw(sprite); 

        

        window.draw(mainRect);
        line.setSize({ thinLineWidth, (float)gridSize });
        for (int i = 1; i < 10; ++i) 
        {
            line.setPosition(gridX + (float)(i * squareSize), gridY);
            window.draw(line);
        }

        line.setSize({ (float)gridSize, thinLineWidth });

        for (int i = 1; i < 10; ++i) 
        {
            line.setPosition(gridX, gridY + (float)(i * squareSize));
            window.draw(line);
        }

        window.draw(leftSidePanelSprite);

        window.draw(selectedSquare);

        for (int i = 0; i < HEIGHT_MAP; ++i) 
        {
            for (int j = 0; j < WIDTH_MAP; ++j)
            {
                int tileIndex = tileMap[i][j] - 1;
                if (tileIndex >= 0 && tileIndex < 6) 
                {
                    s_clothes.setTextureRect(sf::IntRect(tileIndex * 64, 0, 64, 64));
                    s_clothes.setPosition(startX + j * 64, startY + i * 64);
                    window.draw(s_clothes);
                }
            }
        }

        if (dragging && selectedX != -1 && selectedY != -1)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            selectedSquare.setPosition((float)(mousePos.x - squareSize / 2), (float)(mousePos.y - squareSize / 2)); 
            window.draw(selectedSquare);
        }

        window.display();
    }

}
