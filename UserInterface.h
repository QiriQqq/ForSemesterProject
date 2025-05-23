#pragma once

#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include "Calculations.h" // Включаем Calculations.h для доступа к State

#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

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
    static constexpr float INPUT_FIELD_WIDTH = 180.f;
    static constexpr float INPUT_ROW_HEIGHT = 30.f;
    static constexpr float PANEL_PADDING = 10.f;
    static constexpr float WIDGET_SPACING = 10.f;
    static constexpr float HEADER_HEIGHT = 30.f;
    static constexpr float TITLE_HEIGHT = 30.f; // Увеличил для лучшей читаемости заголовков
    static constexpr float SCROLLBAR_WIDTH_ESTIMATE = 18.f;

    void initializeGui();
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
    void drawTrajectoryOnCanvas(sf::RenderTarget& target_rt); // Изменено имя аргумента
    void prepareTrajectoryForDisplay();

    sf::RenderWindow m_window;
    tgui::Gui m_gui;

    tgui::Label::Ptr m_inputTitleLabel;
    tgui::EditBox::Ptr m_edit_m;
    tgui::EditBox::Ptr m_edit_M;
    tgui::EditBox::Ptr m_edit_V0;
    tgui::EditBox::Ptr m_edit_T;
    tgui::EditBox::Ptr m_edit_k;
    tgui::EditBox::Ptr m_edit_F;
    tgui::Button::Ptr m_calculateButton;
    tgui::Grid::Ptr m_inputControlsGrid;

    tgui::Panel::Ptr m_leftPanel;
    tgui::Panel::Ptr m_rightPanel;
    tgui::Panel::Ptr m_trajectoryContainerPanel;
    tgui::Panel::Ptr m_tableContainerPanel;

    tgui::Label::Ptr m_trajectoryTitleLabel;
    tgui::Canvas::Ptr m_trajectoryCanvas;
    sf::Font m_sfmlFont;

    std::vector<TableRowData> m_currentTableData;
    std::vector<State> m_calculatedStates;
    std::vector<sf::Vertex> m_trajectoryDisplayPoints; // Остается для отрисовки
    bool m_trajectoryAvailable;

    // View для канваса, который будет автоматически подгоняться
    sf::View m_fittedCanvasView;

    tgui::Label::Ptr m_tableTitleLabel;
    tgui::Grid::Ptr m_tableHeaderGrid;
    tgui::ScrollablePanel::Ptr m_tableDataPanel;
    tgui::Grid::Ptr m_tableDataGrid;
};

#endif USERINTERFACE_H