# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(BigWashGame)

# Установка стандарта C++ (рекомендуется C++17 или выше)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Добавление исполняемого файла
add_executable(BigWashGame
    main.cpp
)

# Подключение SFML к проекту
target_link_libraries(BigWashGame
    sfml-graphics
    sfml-window
    sfml-system
)

file(COPY "pictures" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "fonts" DESTINATION "${CMAKE_BINARY_DIR}")