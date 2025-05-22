#pragma once

#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp> // Используем <TGUI/TGUI.hpp> для TGUI 0.9.x

#include <vector>
#include <string>
#include <iomanip> // Для std::fixed, std::setprecision в .cpp
#include <sstream> // Для std::stringstream в .cpp

// Структура для данных таблицы
struct TableRowData {
    float h_sec;
    float x, y;
    float Vx, Vy;
};

class UserInterface {
public:
    UserInterface();
    void run();

private:
    // Константы для компоновки и внешнего вида
    // Левая панель
    static constexpr float INPUT_FIELD_WIDTH = 180.f;   // Ширина окна ввода
    static constexpr float INPUT_ROW_HEIGHT = 30.f;     // Высота окна ввода
    static constexpr float PANEL_PADDING = 10.f;        // Общий отступ для панелей
    static constexpr float WIDGET_SPACING = 10.f;       // Пространство между виджетами

    // Правая панель
    static constexpr float HEADER_HEIGHT = 30.f;
    static constexpr float TITLE_HEIGHT = 18.f;
    static constexpr float SCROLLBAR_WIDTH_ESTIMATE = 18.f; // Примерная ширина скроллбара


    void initializeGui();

    // Разделяем loadWidgets для лучшей читаемости
    void loadWidgets();
    void loadLeftPanelWidgets();
    void loadRightPanelWidgets();
    void loadTrajectoryWidgets(tgui::Panel::Ptr parentPanel);
    void loadTableWidgets(tgui::Panel::Ptr parentPanel);

    void setupLayout();
    void connectSignals();

    void handleEvents();
    void update();
    void render();

    void onCalculateButtonPressed();
    void populateTable(const std::vector<TableRowData>& data);
    void drawTrajectoryOnCanvas(sf::RenderTarget& target);

    sf::RenderWindow m_window;
    tgui::Gui m_gui;
    // tgui::Font m_font; // Убрали, используем m_gui.setFont("path")

    // Виджеты ввода
    tgui::Label::Ptr m_inputTitleLabel;
    tgui::EditBox::Ptr m_edit_m;
    tgui::EditBox::Ptr m_edit_M;
    tgui::EditBox::Ptr m_edit_V0;
    tgui::EditBox::Ptr m_edit_T;
    tgui::EditBox::Ptr m_edit_k;
    tgui::EditBox::Ptr m_edit_F;
    tgui::Button::Ptr m_calculateButton;
    tgui::Grid::Ptr m_inputControlsGrid; // Сделаем грид полей ввода членом класса

    // Панели для компоновки
    tgui::Panel::Ptr m_leftPanel;
    tgui::Panel::Ptr m_rightPanel;
    tgui::Panel::Ptr m_trajectoryContainerPanel;
    tgui::Panel::Ptr m_tableContainerPanel;

    // Область отрисовки траектории
    tgui::Label::Ptr m_trajectoryTitleLabel;
    tgui::Canvas::Ptr m_trajectoryCanvas;
    sf::Font m_sfmlFont;
    std::vector<sf::Vertex> m_trajectoryPoints;
    bool m_trajectoryAvailable;

    // Таблица координат
    tgui::Label::Ptr m_tableTitleLabel;
    tgui::Grid::Ptr m_tableHeaderGrid;
    tgui::ScrollablePanel::Ptr m_tableDataPanel;
    tgui::Grid::Ptr m_tableDataGrid;

    std::vector<TableRowData> m_currentTableData;
};

#endif // USERINTERFACE_H