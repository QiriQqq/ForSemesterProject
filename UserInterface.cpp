#include "UserInterface.h"
#include "TrajectoryVisualizer.h"

#include <iostream> // ��� �������
#include <algorithm> // ��� std::min_element, std::max_element
#include <cmath> // ��� std::pow, std::sqrt

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

// --- ��������������� ������� ��� �������� ������ ����� ---
std::pair<tgui::Label::Ptr, tgui::EditBox::Ptr> createInputRowControls(const sf::String& labelText, float editBoxWidth, float rowHeight) {
    auto label = tgui::Label::create(tgui::String(labelText)); // sf::String � L"" ������ �������� � tgui::Label
    if (label) {
        label->getRenderer()->setTextColor(tgui::Color::Black);
        label->setVerticalAlignment(tgui::Label::VerticalAlignment::Center);
    }

    auto editBox = tgui::EditBox::create();
    if (editBox) {
        editBox->setSize({ editBoxWidth, rowHeight });
    }
    return { label, editBox };
}

// --- ����������� � ������������� ---
UserInterface::UserInterface()
    : m_window({ 1200, 800 }, L"������ ���������� �������� ����"),
    m_gui(m_window),
    m_trajectoryAvailable(false) {

    m_gui.setFont("arial.ttf");

    // �������� ������ ��� SFML (������������ �� Canvas)
    if (!m_sfmlFont.loadFromFile("arial.ttf")) {
        std::cerr << "SFML: Error - Failed to load font 'arial.ttf' for SFML rendering!\n";
    }

    initializeGui();
}

void UserInterface::initializeGui() {
    std::cout << "DEBUG: Initializing GUI..." << std::endl;
    loadWidgets();
    setupLayout(); // �������� setupLayout ����� loadWidgets
    connectSignals();
    populateTable({}); // ��������� ������ ��������� �������
    std::cout << "DEBUG: GUI Initialized." << std::endl;
}

// --- �������� �������� ---
void UserInterface::loadWidgets() {
    std::cout << "DEBUG: Loading all widgets..." << std::endl;
    loadLeftPanelWidgets();
    loadRightPanelWidgets();
    std::cout << "DEBUG: All widgets loaded." << std::endl;
}

void UserInterface::loadLeftPanelWidgets() {
    m_leftPanel = tgui::Panel::create();
    if (!m_leftPanel) { std::cerr << "Error: Failed to create m_leftPanel" << std::endl; return; }
    m_leftPanel->getRenderer()->setBackgroundColor(tgui::Color(220, 220, 220));
    m_leftPanel->getRenderer()->setBorders({ 1, 1, 1, 1 });
    m_leftPanel->getRenderer()->setBorderColor(tgui::Color::Black);
    m_gui.add(m_leftPanel);

    // 1. ��������� "�������� ��������"
    m_inputTitleLabel = tgui::Label::create(L"�������� ��������");
    if (!m_inputTitleLabel) { std::cerr << "Error: Failed to create m_inputTitleLabel" << std::endl; return; }
    m_inputTitleLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
    m_inputTitleLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
    m_inputTitleLabel->getRenderer()->setTextColor(tgui::Color::Black);
    m_inputTitleLabel->setSize({ "100% - " + tgui::String::fromNumber(2 * PANEL_PADDING), TITLE_HEIGHT });
    m_inputTitleLabel->setPosition({ PANEL_PADDING, PANEL_PADDING });
    m_leftPanel->add(m_inputTitleLabel);

    // 2. Grid ��� ����� �����
    m_inputControlsGrid = tgui::Grid::create(); // ���������� ���� ������
    if (!m_inputControlsGrid) { std::cerr << "Error: Failed to create m_inputControlsGrid" << std::endl; return; }
    m_inputControlsGrid->setPosition({ PANEL_PADDING, tgui::bindBottom(m_inputTitleLabel) + WIDGET_SPACING });
    m_leftPanel->add(m_inputControlsGrid);

    unsigned int currentRow = 0;
    // ������ ��� ���������� ������ � inputControlsGrid
    auto addInputRowToGrid = [&](const sf::String& text, tgui::EditBox::Ptr& editBoxMember) {
        auto pair = createInputRowControls(text, INPUT_FIELD_WIDTH, INPUT_ROW_HEIGHT);
        if (!pair.first || !pair.second) {
            std::cerr << "Error: Failed to create input pair for: " << text.toAnsiString() << std::endl;
            return;
        }
        editBoxMember = pair.second;
        m_inputControlsGrid->addWidget(pair.first, currentRow, 0);
        m_inputControlsGrid->addWidget(editBoxMember, currentRow, 1);
        m_inputControlsGrid->setWidgetPadding(currentRow, 0, { 5, 5, 5, 0 });  // Label: T,R,B,L (0 �����, �.�. ���� ����� ���� ������)
        m_inputControlsGrid->setWidgetPadding(currentRow, 1, { 5, 0, 5, 5 });  // EditBox: T,R,B,L (0 ������)
        currentRow++;
    };

    addInputRowToGrid(L"m (�����, ��):", m_edit_m);
    addInputRowToGrid(L"M (�����, ��):", m_edit_M);
    addInputRowToGrid(L"V0 (��������, �/�):", m_edit_V0);
    addInputRowToGrid(L"T (�����, ���):", m_edit_T);
    addInputRowToGrid(L"k (������������):", m_edit_k);
    addInputRowToGrid(L"F (������������):", m_edit_F);

    // 3. ������ "���������� ����������!"
    m_calculateButton = tgui::Button::create(L"���������� ����������!");
    if (!m_calculateButton) { std::cerr << "Error: Failed to create m_calculateButton" << std::endl; return; }
    m_calculateButton->getRenderer()->setRoundedBorderRadius(15);
    m_calculateButton->setSize({ "100% - " + tgui::String::fromNumber(2 * PANEL_PADDING), 40 });
    m_calculateButton->setPosition({ PANEL_PADDING, tgui::bindBottom(m_inputControlsGrid) + WIDGET_SPACING * 1.5f }); // ������� ������ ��� ������
    m_leftPanel->add(m_calculateButton);


    // 4. ����� ������ "������� ������������"
    m_showVisualizerButton = tgui::Button::create(L"������� 3D ������������"); // ����� ����� ��������
    if (!m_showVisualizerButton) { std::cerr << "Error: Failed to create m_showVisualizerButton" << std::endl; return; }
    m_showVisualizerButton->getRenderer()->setRoundedBorderRadius(15);
    m_showVisualizerButton->setSize({ "100% - " + tgui::String::fromNumber(2 * PANEL_PADDING), 40 });
    // ������������� ������������ ���������� ������
    m_showVisualizerButton->setPosition({ PANEL_PADDING, tgui::bindBottom(m_calculateButton) + WIDGET_SPACING / 2.0f });
    m_leftPanel->add(m_showVisualizerButton);
}

void UserInterface::loadRightPanelWidgets() {
    m_rightPanel = tgui::Panel::create();
    if (!m_rightPanel) { std::cerr << "Error: Failed to create m_rightPanel" << std::endl; return; }
    m_gui.add(m_rightPanel); // ������� ���������, ����� ����������� ����������

    loadTrajectoryWidgets(m_rightPanel);
    loadTableWidgets(m_rightPanel);
}

void UserInterface::loadTrajectoryWidgets(tgui::Panel::Ptr parentPanel) {
    m_trajectoryContainerPanel = tgui::Panel::create();
    if (!m_trajectoryContainerPanel) { std::cerr << "Error: Failed to create m_trajectoryContainerPanel" << std::endl; return; }
    m_trajectoryContainerPanel->getRenderer()->setBorders({ 1,1,1,1 });
    m_trajectoryContainerPanel->getRenderer()->setBorderColor(tgui::Color::Black);
    m_trajectoryContainerPanel->getRenderer()->setBackgroundColor(tgui::Color::White);
    parentPanel->add(m_trajectoryContainerPanel);

    m_trajectoryTitleLabel = tgui::Label::create(L"���������� �������� ����");
    if (!m_trajectoryTitleLabel) { std::cerr << "Error: Failed to create m_trajectoryTitleLabel" << std::endl; return; }
    m_trajectoryTitleLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
    m_trajectoryTitleLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
    m_trajectoryTitleLabel->getRenderer()->setTextColor(tgui::Color::Black);
    m_trajectoryTitleLabel->setSize({ "100%", TITLE_HEIGHT });
    m_trajectoryContainerPanel->add(m_trajectoryTitleLabel, "TrajectoryTitle"); // ���������� ��� ��� ���������������� �������

    m_trajectoryCanvas = tgui::Canvas::create();
    if (!m_trajectoryCanvas) { std::cerr << "Error: Failed to create m_trajectoryCanvas" << std::endl; return; }
    m_trajectoryCanvas->setSize({ "100%", "100% - " + tgui::String::fromNumber(TITLE_HEIGHT) });
    m_trajectoryCanvas->setPosition({ 0, "TrajectoryTitle.bottom" });
    m_trajectoryContainerPanel->add(m_trajectoryCanvas);
}

void UserInterface::loadTableWidgets(tgui::Panel::Ptr parentPanel) {
    m_tableContainerPanel = tgui::Panel::create();
    if (!m_tableContainerPanel) { std::cerr << "Error: Failed to create m_tableContainerPanel" << std::endl; return; }
    m_tableContainerPanel->getRenderer()->setBorders({ 1,1,1,1 });
    m_tableContainerPanel->getRenderer()->setBorderColor(tgui::Color::Black);
    m_tableContainerPanel->getRenderer()->setBackgroundColor(tgui::Color::White);
    parentPanel->add(m_tableContainerPanel);

    m_tableTitleLabel = tgui::Label::create(L"������� ��������� � ���������");
    if (!m_tableTitleLabel) { std::cerr << "Error: Failed to create m_tableTitleLabel" << std::endl; return; }
    m_tableTitleLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
    m_tableTitleLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
    m_tableTitleLabel->getRenderer()->setTextColor(tgui::Color::Black);
    m_tableTitleLabel->setSize({ "100%", TITLE_HEIGHT });
    m_tableContainerPanel->add(m_tableTitleLabel, "TableTitle");

    m_tableHeaderGrid = tgui::Grid::create();
    if (!m_tableHeaderGrid) { std::cerr << "Error: Failed to create m_tableHeaderGrid" << std::endl; return; }
    m_tableHeaderGrid->setSize({ "100% - " + tgui::String::fromNumber(SCROLLBAR_WIDTH_ESTIMATE), HEADER_HEIGHT });
    m_tableHeaderGrid->setPosition({ 0, "TableTitle.bottom" });

    std::vector<sf::String> headers = { L"h, ���", L"x", L"y", L"Vx", L"Vy" };
    for (size_t i = 0; i < headers.size(); ++i) {
        auto headerLabel = tgui::Label::create(tgui::String(headers[i]));
        if (!headerLabel) { std::cerr << "Error: Failed to create headerLabel " << i << std::endl; continue; }
        
        headerLabel->getRenderer()->setTextColor(tgui::Color::Black);
        headerLabel->getRenderer()->setBorders({ 0,0,0,1 }); // ������ ������ �������
        headerLabel->getRenderer()->setBorderColor(tgui::Color::Black);
        headerLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
        headerLabel->setVerticalAlignment(tgui::Label::VerticalAlignment::Center);
        m_tableHeaderGrid->addWidget(headerLabel, 0, i);
        
        // ����� �������� ������� ��� headerLabel, ���� �����
        // m_tableHeaderGrid->setWidgetPadding(0, i, {2,5,2,5}); // T,R,B,L
    }
    m_tableContainerPanel->add(m_tableHeaderGrid);

    m_tableDataPanel = tgui::ScrollablePanel::create();
    if (!m_tableDataPanel) { std::cerr << "Error: Failed to create m_tableDataPanel" << std::endl; return; }
    m_tableDataPanel->setSize({ "100%", "100% - " + tgui::String::fromNumber(TITLE_HEIGHT + HEADER_HEIGHT) });
    m_tableDataPanel->setPosition({ 0, tgui::bindBottom(m_tableHeaderGrid) });
    m_tableDataPanel->getRenderer()->setBackgroundColor(tgui::Color(245, 245, 245));
    m_tableContainerPanel->add(m_tableDataPanel);

    m_tableDataGrid = tgui::Grid::create();
    if (!m_tableDataGrid) { std::cerr << "Error: Failed to create m_tableDataGrid for data" << std::endl; return; }
    // ������ � ������ m_tableDataGrid ����� ����������� ScrollablePanel � ��� ����������
    m_tableDataPanel->add(m_tableDataGrid);
}

// --- ���������� ---
void UserInterface::setupLayout() {
    std::cout << "DEBUG: Setting up layout..." << std::endl;
    // ����� ������
    m_leftPanel->setSize({ "30%", "100%" }); // ������� ���� ��� ��������
    m_leftPanel->setPosition({ 0, 0 });

    // ������ ������
    m_rightPanel->setSize({ "70%", "100%" });
    m_rightPanel->setPosition({ "30%", 0 });

    // ���������� ������ ������ ������
    const float rightPanelPadding = PANEL_PADDING;
    const float verticalSpacing = WIDGET_SPACING / 2.f;

    m_trajectoryContainerPanel->setSize(
        { tgui::bindWidth(m_rightPanel) - 2 * rightPanelPadding,
         "60% - " + tgui::String::fromNumber(rightPanelPadding + verticalSpacing / 2.f) 
        }
    );
    m_trajectoryContainerPanel->setPosition({ rightPanelPadding, rightPanelPadding });

    m_tableContainerPanel->setSize(
        { tgui::bindWidth(m_rightPanel) - 2 * rightPanelPadding,
         "40% - " + tgui::String::fromNumber(rightPanelPadding + verticalSpacing / 2.f) 
        }
    );
    m_tableContainerPanel->setPosition({ rightPanelPadding, tgui::bindBottom(m_trajectoryContainerPanel) + verticalSpacing });
    std::cout << "DEBUG: Layout setup finished." << std::endl;
}

// --- ����������� �������� ---
void UserInterface::connectSignals() {
    if (m_calculateButton) {
        // ���������� .connect() ��� TGUI 0.9.x
        m_calculateButton->onPress.connect(&UserInterface::onCalculateButtonPressed, this);
    }
    else {
        std::cerr << "Error: m_calculateButton is null in connectSignals! Cannot connect." << std::endl;
    }

    // ���������� ������ ��� ����� ������
    if (m_showVisualizerButton) {
        m_showVisualizerButton->onPress.connect(&UserInterface::onShowVisualizerButtonPressed, this);
    }
    else {
        std::cerr << "Error: m_showVisualizerButton is null in connectSignals! Cannot connect." << std::endl;
    }
}

// --- ����������� � ������ ---
void UserInterface::onCalculateButtonPressed() {
    std::cout << "Calculate button pressed!" << std::endl;

    // ���������� ���������
    const double G_SI = 6.67430e-11; // �^3 ��^-1 �^-2
    const double REFERENCE_PHYSICAL_LENGTH_FOR_X_1_5 = 1.495978707e11; // 1 AU � ������
    const double SECONDS_PER_DAY = 24.0 * 60.0 * 60.0;

    SimulationParameters paramsFromUI;

    double M_ui_val_from_editbox = 1.0;
    double m_satellite_ui_kg = 100.0;    // ����� ��������, �� (�������� �� ���������)
    double V0_ui_si = 0.0;
    double T_total_ui_days = 1.0;
    double k_val_input = paramsFromUI.DRAG_COEFFICIENT;
    double F_val_input = paramsFromUI.THRUST_COEFFICIENT;

    try {
        if (m_edit_M && !m_edit_M->getText().empty())
            M_ui_val_from_editbox = std::stod(m_edit_M->getText().toStdString());
        else {
            std::cerr << "Warning: Central body mass (M) is empty. Using default 1.0 for input value." << std::endl;
            M_ui_val_from_editbox = 1.0;
        }

        // ������ ����� �������� m
        if (m_edit_m && !m_edit_m->getText().empty())
            m_satellite_ui_kg = std::stod(m_edit_m->getText().toStdString());
        else {
            std::cerr << "Warning: Satellite mass (m) is empty. Using default 100.0 kg." << std::endl;
            // m_satellite_ui_kg ��� 100.0
        }

        if (m_satellite_ui_kg < 0) { // �������� �� ������������� ����� ��������
            std::cerr << "Error: Satellite mass (m) cannot be negative." << std::endl;
            if (m_inputTitleLabel) m_inputTitleLabel->setText(L"����� �������� >= 0!");
            m_trajectoryAvailable = false; m_calculatedStates.clear();
            prepareTrajectoryForDisplay(); populateTable({});
            return;
        }


        if (m_edit_V0 && !m_edit_V0->getText().empty())
            V0_ui_si = std::stod(m_edit_V0->getText().toStdString());

        if (m_edit_T && !m_edit_T->getText().empty())
            T_total_ui_days = std::stod(m_edit_T->getText().toStdString());

        if (m_edit_k && !m_edit_k->getText().empty())
            k_val_input = std::stod(m_edit_k->getText().toStdString());

        if (m_edit_F && !m_edit_F->getText().empty())
            F_val_input = std::stod(m_edit_F->getText().toStdString());

    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing input values: " << e.what() << std::endl;
        if (m_inputTitleLabel) m_inputTitleLabel->setText(L"������ ����� ����������!");
        m_trajectoryAvailable = false; m_calculatedStates.clear();
        prepareTrajectoryForDisplay(); populateTable({});
        return;
    }
    if (m_inputTitleLabel) m_inputTitleLabel->setText(L"�������� ��������");

    // 1. ��������� ����������� � M_central � ���������� ��������
    double M_central_body_physical_kg = M_ui_val_from_editbox * 1.0e25;

    // �������� ������� ����� ��� ���������������:
    double mass_unit_for_scaling = M_central_body_physical_kg;

    double length_unit = REFERENCE_PHYSICAL_LENGTH_FOR_X_1_5 / paramsFromUI.initialState.x;

    if (mass_unit_for_scaling <= 1e-9) {
        std::cerr << "Error: Scaling mass unit (M_central_body_physical_kg) must be significantly positive." << std::endl;
        if (m_inputTitleLabel) m_inputTitleLabel->setText(L"����� �����. ���� > 0!");
        m_trajectoryAvailable = false; m_calculatedStates.clear();
        prepareTrajectoryForDisplay(); populateTable({});
        return;
    }
    // ������� ������� ������� �� G_SI � ��������� ������� ����� ��� ���������������
    double time_unit = std::sqrt(std::pow(length_unit, 3) / (G_SI * mass_unit_for_scaling));

    std::cout << "DEBUG SCALES: mass_unit_for_scaling=" << mass_unit_for_scaling << " kg, length_unit=" << length_unit << " m, time_unit=" << time_unit << " s" << std::endl;
    std::cout << "DEBUG MASSES: M_central_phys=" << M_central_body_physical_kg << " kg, m_satellite_phys=" << m_satellite_ui_kg << " kg" << std::endl;

    // 2. ���������� ��������� paramsFromUI ������������� ����������

    paramsFromUI.G = 1.0;
    paramsFromUI.M = (M_central_body_physical_kg + m_satellite_ui_kg) / mass_unit_for_scaling;
    // ��� ������������: 1.0 + (m_satellite_ui_kg / M_central_body_physical_kg)

    std::cout << "DEBUG PARAMS: G_calc=" << paramsFromUI.G << ", M_calc_eff_for_gravity=" << paramsFromUI.M << std::endl;

    // ������������ k � F:
    paramsFromUI.DRAG_COEFFICIENT = k_val_input;
    paramsFromUI.THRUST_COEFFICIENT = F_val_input;
    std::cout << "DEBUG PARAMS: DRAG_COEFF_calc=" << paramsFromUI.DRAG_COEFFICIENT << ", THRUST_COEFF_calc=" << paramsFromUI.THRUST_COEFFICIENT << std::endl;


    // ������ STEPS � ��������� �������� vy
    double T_total_ui_sec = T_total_ui_days * SECONDS_PER_DAY;
    double T_total_dimensionless = T_total_ui_sec / time_unit;

    if (paramsFromUI.DT > 1e-9) {
        paramsFromUI.STEPS = static_cast<int>(T_total_dimensionless / paramsFromUI.DT);
    }
    else {
        paramsFromUI.STEPS = 1000;
        std::cerr << "Warning: DT is too small or zero. Using default STEPS." << std::endl;
    }
    if (paramsFromUI.STEPS <= 0) paramsFromUI.STEPS = 1;

    std::cout << "DEBUG PARAMS: T_total_dimless=" << T_total_dimensionless << ", STEPS=" << paramsFromUI.STEPS << std::endl;

    double characteristic_velocity = length_unit / time_unit;
    if (std::abs(characteristic_velocity) > 1e-9) {
        paramsFromUI.initialState.vy = V0_ui_si / characteristic_velocity;
    }
    else {
        paramsFromUI.initialState.vy = 0.0;
        std::cerr << "Warning: Characteristic velocity (length_unit/time_unit) is near zero. Setting vy_dimless to 0." << std::endl;
    }
    std::cout << "DEBUG PARAMS: vy_dimless=" << paramsFromUI.initialState.vy << std::endl;

    // --- ��������� ����� ������ (����� calculator.runSimulation � �.�.) ��� ��������� ---
    Calculations calculator;
    m_calculatedStates = calculator.runSimulation(paramsFromUI);

    m_currentTableData.clear();
    if (!m_calculatedStates.empty()) {
        m_trajectoryAvailable = true;

        const size_t maxTableEntries = 100;
        size_t step_size_for_table = 1;

        if (m_calculatedStates.size() > maxTableEntries) {
            step_size_for_table = m_calculatedStates.size() / maxTableEntries;
            if (step_size_for_table == 0) step_size_for_table = 1;
        }

        for (size_t i = 0; i < m_calculatedStates.size(); i += step_size_for_table) {
            const auto& state = m_calculatedStates[i];
            double current_dimensionless_time = i * paramsFromUI.DT;
            double current_physical_time_sec = current_dimensionless_time * time_unit;
            double current_physical_time_days = current_physical_time_sec / SECONDS_PER_DAY;

            m_currentTableData.push_back({
                static_cast<float>(current_physical_time_days),
                static_cast<float>(state.x),
                static_cast<float>(state.y),
                static_cast<float>(state.vx),
                static_cast<float>(state.vy)
                });
        }
    }
    else {
        m_trajectoryAvailable = false;
    }

    prepareTrajectoryForDisplay();
    populateTable(m_currentTableData);
}




void UserInterface::prepareTrajectoryForDisplay() {
    m_trajectoryDisplayPoints.clear();
    if (!m_trajectoryAvailable || m_calculatedStates.empty()) {
        std::cout << "DEBUG: No trajectory to prepare for display." << std::endl;
        return;
    }

    m_trajectoryDisplayPoints.reserve(m_calculatedStates.size());
    for (const auto& state : m_calculatedStates) {
        m_trajectoryDisplayPoints.emplace_back(
            sf::Vector2f(static_cast<float>(state.x), static_cast<float>(-state.y)), // Y ������������� ��� �����������
            sf::Color::Blue // ���� ����� ����������
        );
    }
    std::cout << "DEBUG: Trajectory display points prepared. Count: " << m_trajectoryDisplayPoints.size() << std::endl;
}

void UserInterface::drawTrajectoryOnCanvas(sf::RenderTarget& canvasRenderTarget) {
    sf::View originalView = canvasRenderTarget.getView();
    sf::View fittedView;

    if (m_trajectoryAvailable && !m_trajectoryDisplayPoints.empty()) {

        // 1. ������������ �������������� ������������� (bounding box) ��� ����� �����������
        //    (����� ���������� + ������ ��������� (0,0) ��� ������������ ����)
        float min_x_content = m_trajectoryDisplayPoints[0].position.x;
        float max_x_content = m_trajectoryDisplayPoints[0].position.x;
        float min_y_content = m_trajectoryDisplayPoints[0].position.y; // ����� <= 0
        float max_y_content = m_trajectoryDisplayPoints[0].position.y; // ����� >= min_y_content, ����� ���� > 0 ���� ���������� ���������� y_sim=0

        for (const auto& vertex : m_trajectoryDisplayPoints) {
            min_x_content = std::min(min_x_content, vertex.position.x);
            max_x_content = std::max(max_x_content, vertex.position.x);
            min_y_content = std::min(min_y_content, vertex.position.y);
            max_y_content = std::max(max_y_content, vertex.position.y);
        }

        // ��������, ��� (0,0) �������� � bounding box
        min_x_content = std::min(min_x_content, 0.0f);
        max_x_content = std::max(max_x_content, 0.0f);
        min_y_content = std::min(min_y_content, 0.0f); // ���� ��� y < 0, �� max_y_content ������ 0.0f
        max_y_content = std::max(max_y_content, 0.0f); // ���� ��� y > 0 (�� ��� ������ � ���������), min_y_content ������ 0.0f

        // ������ � ������ ����������� ��� ��������
        float content_width_no_padding = max_x_content - min_x_content;
        float content_height_no_padding = max_y_content - min_y_content; // ������������� �����

        // 2. ��������� ������� (padding)
        float paddingFactor = 0.1f; // 10% ������

        // ������� ������� ��� ������� ����������� �������.
        // ���� ������� ����� ��������� (�����), ����� ����������� ���������� ������.
        const float MIN_DIM_FOR_PERCENT_PADDING = 0.1f; // ���� ������ ������ �����, ������ ����� �� ����� ��������
        float base_width_for_padding = std::max(content_width_no_padding, MIN_DIM_FOR_PERCENT_PADDING);
        float base_height_for_padding = std::max(content_height_no_padding, MIN_DIM_FOR_PERCENT_PADDING);

        float padding_x = base_width_for_padding * paddingFactor;
        float padding_y = base_height_for_padding * paddingFactor;

        // ���������� ���������������� ��������������
        float padded_min_x = min_x_content - padding_x;
        float padded_max_x = max_x_content + padding_x;
        float padded_min_y = min_y_content - padding_y; // ������ ��� ����� �������������
        float padded_max_y = max_y_content + padding_y; // ������ ��� ����� ������������� (��� ������ �� ����, ���� max_y_content ��� <0)

        // ����������� ������� ���������������� ��������
        float actual_padded_content_width = padded_max_x - padded_min_x;
        float actual_padded_content_height = padded_max_y - padded_min_y;

        // 3. ���������� "�����������" ������� �������� ��� ������� View.
        //    ��� �����, ����� �������� ������� �� ���� ��� ������� ��������� �������� View.
        const float MIN_EFFECTIVE_VIEW_DIMENSION = 0.02f; // ����������� ������ ������� View � ������� ����������� (���� ������ ������� ����)
        float effective_view_content_width = std::max(actual_padded_content_width, MIN_EFFECTIVE_VIEW_DIMENSION);
        float effective_view_content_height = std::max(actual_padded_content_height, MIN_EFFECTIVE_VIEW_DIMENSION);

        // 4. ������������ ������� sf::View, ����� �� �������������� ����������� ������ �������
        //    � ������ effective_view_content_width/height.
        sf::Vector2u canvasSize = canvasRenderTarget.getSize();
        if (canvasSize.x == 0 || canvasSize.y == 0) {
            canvasRenderTarget.setView(originalView); // ������ ������� �������, ������ �� ������
            return;
        }
        float canvasAspectRatio = static_cast<float>(canvasSize.x) / canvasSize.y;
        float effectiveContentAspectRatio = effective_view_content_width / effective_view_content_height;

        float view_width_world;  // �������� ������ View � ������� �����������
        float view_height_world; // �������� ������ View � ������� �����������

        if (canvasAspectRatio > effectiveContentAspectRatio) {
            view_height_world = effective_view_content_height;
            view_width_world = view_height_world * canvasAspectRatio;
        }
        else {
            view_width_world = effective_view_content_width;
            view_height_world = view_width_world / canvasAspectRatio;
        }
        fittedView.setSize(view_width_world, view_height_world);

        // 5. ���������� sf::View �� ������ *������������* ���������������� ��������.
        //    ��� �������� ������. ����� ������ ���� �� actual_padded_*, � �� effective_*.
        sf::Vector2f actual_padded_content_center(
            padded_min_x + actual_padded_content_width / 2.0f,
            padded_min_y + actual_padded_content_height / 2.0f
        );
        fittedView.setCenter(actual_padded_content_center);

        canvasRenderTarget.setView(fittedView);

        // --- ��������� ---
        const float actual_central_body_radius = 0.01f; // ���������� ������
        sf::CircleShape centerBody(actual_central_body_radius);
        centerBody.setFillColor(sf::Color::Red);
        centerBody.setOrigin(actual_central_body_radius, actual_central_body_radius);
        centerBody.setPosition(0.f, 0.f);
        canvasRenderTarget.draw(centerBody);

        // m_trajectoryDisplayPoints ��� �������� �� !empty() � ������
        canvasRenderTarget.draw(m_trajectoryDisplayPoints.data(), m_trajectoryDisplayPoints.size(), sf::LineStrip);
    }
    else {
        // ... (��� ��� placeholder ������) ...
        canvasRenderTarget.setView(canvasRenderTarget.getDefaultView());
        sf::Text placeholderText;
        if (m_sfmlFont.hasGlyph(L'�')) { // ���������, ���������� �� �����
            placeholderText.setFont(m_sfmlFont);
            placeholderText.setString(L"���������� �� ����������.\n������� '���������� ����������!'");
        }
        else {
            placeholderText.setString("Trajectory not calculated.\nPress 'Calculate Trajectory!' (Font error)"); // �������
        }
        placeholderText.setCharacterSize(16);
        placeholderText.setFillColor(sf::Color(105, 105, 105));
        sf::FloatRect textRect = placeholderText.getLocalBounds();
        placeholderText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        placeholderText.setPosition(static_cast<float>(canvasRenderTarget.getSize().x) / 2.0f,
            static_cast<float>(canvasRenderTarget.getSize().y) / 2.0f);
        canvasRenderTarget.draw(placeholderText);
    }
    canvasRenderTarget.setView(originalView); // ��������������� �������� View
}

void UserInterface::onShowVisualizerButtonPressed() {
    std::cout << "Show Visualizer button pressed!" << std::endl;

    if (!m_trajectoryAvailable || m_calculatedStates.empty()) {
        std::cerr << "UserInterface: No trajectory data to visualize. Please calculate first." << std::endl;
        // ����� �������� ������������ ��������� � GUI, ���� �����
        // ��������, �������� �������� ����� m_inputTitleLabel
        if (m_inputTitleLabel) {
            m_inputTitleLabel->setText(L"������� ����������� ����������!");
            // ����� ��������� ������, ����� ����� ���� ������ ������� ����� �������,
            // ��� ������������ ��� ������, ����� ������ "����������".
        }
        return;
    }

    // ����������� m_calculatedStates (std::vector<State>) 
    // � WorldTrajectoryData (std::vector<std::pair<double, double>>)
    WorldTrajectoryData trajectoryForVisualizer;
    trajectoryForVisualizer.reserve(m_calculatedStates.size());
    for (const auto& state : m_calculatedStates) {
        // TrajectoryVisualizer ������� ������������ ���������� x, y,
        // ��� ��� �� ��� ���������� �� ���������������� ��� �����������.
        // ���� m_calculatedStates ��� ������ ������������ x � y.
        trajectoryForVisualizer.emplace_back(state.x, state.y);
    }

    if (trajectoryForVisualizer.empty()) {
        std::cerr << "UserInterface: Conversion to WorldTrajectoryData resulted in empty data." << std::endl;
        return;
    }

    std::cout << "UserInterface: Launching TrajectoryVisualizer with "
        << trajectoryForVisualizer.size() << " points." << std::endl;

    // ������� � ��������� ������������
    // ������� ���� ������������� ����� ������� �������������� ��� ����� �� ��������
    try {
        TrajectoryVisualizer visualizer(1000, 800, "Standalone 2D Trajectory Visualizer");
        visualizer.setData(trajectoryForVisualizer);
        visualizer.run(); // ���� ����� ��������� ���������� �����, ���� ���� visualizer �� ���������
    }
    catch (const std::exception& e) {
        std::cerr << "UserInterface: Exception while running TrajectoryVisualizer: " << e.what() << std::endl;
        // ��������� ������, ���� �������� ��� ������ TrajectoryVisualizer �� �������
    }
    std::cout << "UserInterface: TrajectoryVisualizer window closed." << std::endl;
    // ����� �������� ���� ������������� ����� ������� ����� ��� �������� ���-�� � �������� UI, ���� �����.
    if (m_inputTitleLabel && m_inputTitleLabel->getText() == L"������� ����������� ����������!") {
        m_inputTitleLabel->setText(L"�������� ��������"); // ������� �������� �����, ���� �� ��� �������
    }
}

void UserInterface::populateTable(const std::vector<TableRowData>& data) {
    if (!m_tableDataGrid) { std::cerr << "Error: m_tableDataGrid is null in populateTable!" << std::endl; return; }
    m_tableDataGrid->removeAllWidgets();

    if (data.empty()) {
        auto emptyLabel = tgui::Label::create(L"��� ������ ��� �����������");
        if (emptyLabel) {
            emptyLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
            m_tableDataGrid->addWidget(emptyLabel, 0, 0);
            // ����� emptyLabel ������� ��� 5 �������, ���� API Grid �� ��������� colspan:
            // for (unsigned int j = 1; j < 5; ++j) {
            //     m_tableDataGrid->addWidget(tgui::Label::create(""), 0, j); // ������ �����
            // }
        }
        if (m_tableDataPanel) m_tableDataPanel->setContentSize({ 0,0 });
        return;
    }

    size_t step = 1;
    if (data.size() > 100) { // ������: ���� ����� ������ 100, ���������� �������� 100 �����
        step = data.size() / 100;
    }
    if (step == 0) step = 1; // �� ������ ������

    for (size_t i = 0; i < data.size(); i += step) {
        const auto& rowData = data[i];
        std::stringstream ss_h, ss_x, ss_y, ss_vx, ss_vy;
        ss_h << std::fixed << std::setprecision(2) << rowData.h_days;
        ss_x << std::fixed << std::setprecision(2) << rowData.x;
        ss_y << std::fixed << std::setprecision(2) << rowData.y;
        ss_vx << std::fixed << std::setprecision(2) << rowData.Vx;
        ss_vy << std::fixed << std::setprecision(2) << rowData.Vy;
        std::vector<tgui::String> rowStrings = {
            tgui::String(ss_h.str()), tgui::String(ss_x.str()), tgui::String(ss_y.str()),
            tgui::String(ss_vx.str()), tgui::String(ss_vy.str()) };

        for (size_t j = 0; j < rowStrings.size(); ++j) {
            auto cellLabel = tgui::Label::create(rowStrings[j]);
            if (cellLabel) {
                cellLabel->getRenderer()->setTextColor(tgui::Color::Black);
                cellLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
                m_tableDataGrid->addWidget(cellLabel, i, j);
                m_tableDataGrid->setWidgetPadding(i, j, { 2, 5, 2, 5 });
            }
        }
    }
    if (m_tableDataPanel && m_tableDataGrid) {
        m_tableDataPanel->setContentSize(m_tableDataGrid->getSize());
    }
}

// --- ������� ���� � ��������� ������� ---
void UserInterface::run() {
    m_window.setFramerateLimit(60); // ����������� FPS ��� ��������� � �������� ��������
    while (m_window.isOpen()) {
        handleEvents();
        update();
        render();
    }
}

void UserInterface::handleEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        // 1. ������� �������� ������� � TGUI ��� ��� ���������� ���������.
        //    TGUI ���� ���������� sf::Event::Resized ��� ����� ����,
        //    ���� ����, �� ������� ��� ��������, ����� ����������� View.
        m_gui.handleEvent(event);

        // 2. ����� ���� ���������������� ��������� �������
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        else if (event.type == sf::Event::Resized) {
            // ���� SFML �������� ������.
            // ��������� View ��� ������ ���� SFML.
            // TGUI, ��� ������ m_gui.draw(), ����� �������� � ������� View ����.
            sf::FloatRect visibleArea(0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
            m_window.setView(sf::View(visibleArea));

            // ���������� �����:
            std::cout << "DEBUG: Window Resized to: " << event.size.width << "x" << event.size.height
                << ". SFML Window View updated." << std::endl;
        }
        // ������ ���� ����������� �������
    }
}

void UserInterface::update() {
    // ��������, �������� ��� ������ ���������� ���������, �� ��������� � ������ ������������
}

void UserInterface::render() {
    if (m_trajectoryCanvas) {
        sf::RenderTexture& canvasRT = m_trajectoryCanvas->getRenderTexture();
        sf::Vector2f canvasWidgetSize = m_trajectoryCanvas->getSize(); // ������ ������� TGUI

        if (canvasRT.getSize().x != static_cast<unsigned int>(canvasWidgetSize.x) ||
            canvasRT.getSize().y != static_cast<unsigned int>(canvasWidgetSize.y)) {

            std::cout << "DEBUG: Canvas RenderTexture size (" << canvasRT.getSize().x << "x" << canvasRT.getSize().y
                << ") differs from TGUI Widget size (" << canvasWidgetSize.x << "x" << canvasWidgetSize.y
                << "). Recreating RenderTexture for Canvas." << std::endl;

            if (canvasWidgetSize.x > 0 && canvasWidgetSize.y > 0) {
                if (!canvasRT.create(static_cast<unsigned int>(canvasWidgetSize.x), static_cast<unsigned int>(canvasWidgetSize.y))) {
                    std::cerr << "ERROR: Failed to recreate Canvas RenderTexture!" << std::endl;
                }
            }
            else {
                std::cout << "DEBUG: Canvas widget size is zero, not recreating RenderTexture." << std::endl;
            }
        }

        canvasRT.clear(sf::Color(250, 250, 250)); // ��� �������
        drawTrajectoryOnCanvas(canvasRT);      // ���� ����� ������ ��� ������������� � ���������� View
        m_trajectoryCanvas->display();
    }
    m_window.clear(sf::Color(220, 220, 220));
    m_gui.draw();
    m_window.display();
}