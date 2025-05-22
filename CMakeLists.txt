cmake_minimum_required(VERSION 3.10)
project(TrajectoryUI_SFML_TGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Найти SFML
find_package(SFML 2.6 COMPONENTS system window graphics network audio REQUIRED)

# Найти TGUI (убедитесь, что TGUI установлен и CMake может его найти)
# Возможно, потребуется указать TGUI_DIR, если он не в стандартных путях
find_package(TGUI 1.0 REQUIRED) # Укажите вашу версию TGUI, если отличается (e.g. 0.10 for older)

add_executable(${PROJECT_NAME} main.cpp UserInterface.cpp UserInterface.h)

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system TGUI::tgui) # или TGUI::tgui-sfml-graphics для TGUI 1.x

# Для Windows, если это консольное приложение, которое вы не хотите видеть:
# if(WIN32)
#     set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
# endif()

# Копирование ресурсов (например, шрифта) в директорию сборки (опционально)
# file(COPY arial.ttf DESTINATION ${CMAKE_BINARY_DIR})