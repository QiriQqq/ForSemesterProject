#include "UserInterface.h"
#include <iostream> // Для отладки

#if defined(_MSC_VER) // Для Visual Studio, чтобы строковые литералы были UTF-8
#pragma execution_character_set("utf-8")
#endif

// --- Вспомогательная функция для создания строки ввода ---
// Мы оставим ее глобальной в этом .cpp файле, так как она специфична для UI
// и не требует доступа к членам класса UserInterface напрямую.
// Если бы она была более общей, можно было бы вынести в отдельный utility файл.
std::pair<tgui::Label::Ptr, tgui::EditBox::Ptr> createInputRowControls(const sf::String& labelText, float editBoxWidth, float rowHeight) {
    auto label = tgui::Label::create(tgui::String(labelText)); // sf::String с L"" хорошо работает с tgui::Label
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

// --- Конструктор и инициализация ---
UserInterface::UserInterface()
    : m_window({ 1200, 800 }, L"Расчет траектории движения тела"), // Используем L"" для заголовка окна
    m_gui(m_window),
    m_trajectoryAvailable(false) {

    m_gui.setFont("arial.ttf");

    // Загрузка шрифта для SFML (используется на Canvas)
    if (!m_sfmlFont.loadFromFile("arial.ttf")) {
        std::cerr << "SFML: Error - Failed to load font 'arial.ttf' for SFML rendering!\n";
    }
    else {
        std::cout << "SFML: Font 'arial.ttf' for canvas rendering loaded.\n";
    }

    initializeGui();
}

void UserInterface::initializeGui() {
    std::cout << "DEBUG: Initializing GUI..." << std::endl;
    loadWidgets();
    setupLayout();
    connectSignals();
    populateTable({}); // Начальное пустое состояние таблицы
    std::cout << "DEBUG: GUI Initialized." << std::endl;
}

// --- Загрузка виджетов ---
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

    // 1. Заголовок "Исходные значения"
    m_inputTitleLabel = tgui::Label::create(L"Исходные значения");
    if (!m_inputTitleLabel) { std::cerr << "Error: Failed to create m_inputTitleLabel" << std::endl; return; }
    m_inputTitleLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
    m_inputTitleLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
    m_inputTitleLabel->getRenderer()->setTextColor(tgui::Color::Black);
    m_inputTitleLabel->setSize({ "100% - " + tgui::String::fromNumber(2 * PANEL_PADDING), TITLE_HEIGHT });
    m_inputTitleLabel->setPosition({ PANEL_PADDING, PANEL_PADDING });
    m_leftPanel->add(m_inputTitleLabel);

    // 2. Grid для полей ввода
    m_inputControlsGrid = tgui::Grid::create(); // Используем член класса
    if (!m_inputControlsGrid) { std::cerr << "Error: Failed to create m_inputControlsGrid" << std::endl; return; }
    m_inputControlsGrid->setPosition({ PANEL_PADDING, tgui::bindBottom(m_inputTitleLabel) + WIDGET_SPACING });
    m_leftPanel->add(m_inputControlsGrid);

    unsigned int currentRow = 0;
    // Лямбда для добавления строки в inputControlsGrid
    auto addInputRowToGrid = [&](const sf::String& text, tgui::EditBox::Ptr& editBoxMember) {
        auto pair = createInputRowControls(text, INPUT_FIELD_WIDTH, INPUT_ROW_HEIGHT);
        if (!pair.first || !pair.second) {
            std::cerr << "Error: Failed to create input pair for: " << text.toAnsiString() << std::endl;
            return;
        }
        editBoxMember = pair.second;
        m_inputControlsGrid->addWidget(pair.first, currentRow, 0);
        m_inputControlsGrid->addWidget(editBoxMember, currentRow, 1);
        m_inputControlsGrid->setWidgetPadding(currentRow, 0, { 5, 5, 5, 0 });  // Label: T,R,B,L (0 слева, т.к. грид имеет свой отступ)
        m_inputControlsGrid->setWidgetPadding(currentRow, 1, { 5, 0, 5, 5 });  // EditBox: T,R,B,L (0 справа)
        currentRow++;
    };

    addInputRowToGrid(L"m (масса, кг):", m_edit_m);
    addInputRowToGrid(L"M (масса, кг):", m_edit_M);
    addInputRowToGrid(L"V0 (скорость, м/с):", m_edit_V0);
    addInputRowToGrid(L"T (время, сек):", m_edit_T);
    addInputRowToGrid(L"k (безразмерная):", m_edit_k);
    addInputRowToGrid(L"F (безразмерная):", m_edit_F);

    // 3. Кнопка "Рассчитать траекторию!"
    m_calculateButton = tgui::Button::create(L"Рассчитать траекторию!");
    if (!m_calculateButton) { std::cerr << "Error: Failed to create m_calculateButton" << std::endl; return; }
    m_calculateButton->getRenderer()->setRoundedBorderRadius(15);
    m_calculateButton->setSize({ "100% - " + tgui::String::fromNumber(2 * PANEL_PADDING), 40 });
    m_calculateButton->setPosition({ PANEL_PADDING, tgui::bindBottom(m_inputControlsGrid) + WIDGET_SPACING * 2 }); // Больший отступ для кнопки
    m_leftPanel->add(m_calculateButton);
}

void UserInterface::loadRightPanelWidgets() {
    m_rightPanel = tgui::Panel::create();
    if (!m_rightPanel) { std::cerr << "Error: Failed to create m_rightPanel" << std::endl; return; }
    m_gui.add(m_rightPanel); // Сначала добавляем, потом настраиваем содержимое

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

    m_trajectoryTitleLabel = tgui::Label::create(L"Траектория движения тела");
    if (!m_trajectoryTitleLabel) { std::cerr << "Error: Failed to create m_trajectoryTitleLabel" << std::endl; return; }
    m_trajectoryTitleLabel->getRenderer()->setTextStyle(tgui::TextStyle::Bold);
    m_trajectoryTitleLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
    m_trajectoryTitleLabel->getRenderer()->setTextColor(tgui::Color::Black);
    m_trajectoryTitleLabel->setSize({ "100%", TITLE_HEIGHT });
    m_trajectoryContainerPanel->add(m_trajectoryTitleLabel, "TrajectoryTitle"); // Используем имя для позиционирования канваса

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

    m_tableTitleLabel = tgui::Label::create(L"Таблица координат и скоростей");
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

    std::vector<sf::String> headers = { L"h, сек", L"x", L"y", L"Vx", L"Vy" };
    for (size_t i = 0; i < headers.size(); ++i) {
        auto headerLabel = tgui::Label::create(tgui::String(headers[i]));
        if (!headerLabel) { std::cerr << "Error: Failed to create headerLabel " << i << std::endl; continue; }
        headerLabel->getRenderer()->setTextColor(tgui::Color::Black);
        headerLabel->getRenderer()->setBorders({ 0,0,0,1 }); // Только нижняя граница
        headerLabel->getRenderer()->setBorderColor(tgui::Color::Black);
        headerLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
        headerLabel->setVerticalAlignment(tgui::Label::VerticalAlignment::Center);
        m_tableHeaderGrid->addWidget(headerLabel, 0, i);
        // Можно добавить отступы для headerLabel, если нужно
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
    // Ширина и высота m_tableDataGrid будут управляться ScrollablePanel и его содержимым
    m_tableDataPanel->add(m_tableDataGrid);
}

// --- Компоновка ---
void UserInterface::setupLayout() {
    std::cout << "DEBUG: Setting up layout..." << std::endl;
    // Левая панель
    m_leftPanel->setSize({ "30%", "100%" }); // Немного шире для комфорта
    m_leftPanel->setPosition({ 0, 0 });

    // Правая панель
    m_rightPanel->setSize({ "70%", "100%" });
    m_rightPanel->setPosition({ "30%", 0 });

    // Контейнеры внутри правой панели
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

// --- Подключение сигналов ---
void UserInterface::connectSignals() {
    if (m_calculateButton) {
        // Используем .connect() для TGUI 0.9.x
        m_calculateButton->onPress.connect(&UserInterface::onCalculateButtonPressed, this);
    }
    else {
        std::cerr << "Error: m_calculateButton is null in connectSignals! Cannot connect." << std::endl;
    }
}

// --- Обработчики и логика ---
void UserInterface::onCalculateButtonPressed() {
    std::cout << "Calculate button pressed!" << std::endl;
    m_currentTableData.clear();
    m_trajectoryPoints.clear();
    m_trajectoryAvailable = false; // Сбрасываем флаг

    // Здесь будет реальное считывание и расчет
    // Примерные данные для демонстрации:
    try {
        // Пытаемся считать хотя бы одно значение для примера
        // В реальном коде здесь будет считывание всех m_edit_*
        if (m_edit_m && !m_edit_m->getText().empty()) {
            float m_val = std::stof(m_edit_m->getText().toStdString()); // stof может бросить исключение
            std::cout << "m = " << m_val << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing input: " << e.what() << std::endl;
        // Можно вывести сообщение об ошибке в GUI, например, изменив текст m_inputTitleLabel
        if (m_inputTitleLabel) m_inputTitleLabel->setText(L"Ошибка ввода!");
        return; // Прерываем расчет
    }
    if (m_inputTitleLabel) m_inputTitleLabel->setText(L"Исходные значения"); // Возвращаем нормальный текст

    // --- Заглушка для данных траектории и таблицы ---
    m_trajectoryPoints.emplace_back(sf::Vertex(sf::Vector2f(50, 200), sf::Color::Blue));
    m_trajectoryPoints.emplace_back(sf::Vertex(sf::Vector2f(150, 150), sf::Color::Blue));
    m_trajectoryPoints.emplace_back(sf::Vertex(sf::Vector2f(250, 120), sf::Color::Blue));
    m_trajectoryPoints.emplace_back(sf::Vertex(sf::Vector2f(350, 150), sf::Color::Blue));
    m_trajectoryPoints.emplace_back(sf::Vertex(sf::Vector2f(450, 200), sf::Color::Blue));
    m_trajectoryAvailable = true;

    for (int i = 0; i < 35; ++i) { // Больше данных для теста скролла
        m_currentTableData.push_back({
            0.1f * i,
            100.f + i * 10.f, 200.f - i * 2.5f,
            5.0f - i * 0.1f, -2.0f + i * 0.2f
        });
    }
    populateTable(m_currentTableData);
}

void UserInterface::drawTrajectoryOnCanvas(sf::RenderTarget& canvasRenderTarget) {
    if (m_trajectoryAvailable && !m_trajectoryPoints.empty()) {
        // Простая отрисовка линии
        // Для более сложного масштабирования/панорамирования канваса нужно будет управлять sf::View для canvasRenderTarget
        if (m_trajectoryPoints.size() >= 2) {
            canvasRenderTarget.draw(m_trajectoryPoints.data(), m_trajectoryPoints.size(), sf::LineStrip);
        }
        else if (m_trajectoryPoints.size() == 1) {
            sf::CircleShape point(2.f);
            point.setFillColor(m_trajectoryPoints[0].color);
            point.setPosition(m_trajectoryPoints[0].position);
            point.setOrigin(2.f, 2.f);
            canvasRenderTarget.draw(point);
        }
    }
    else {
        sf::Text placeholderText;
        if (m_sfmlFont.getInfo().family.empty()) {
            placeholderText.setString("SFML Font not loaded for canvas."); // ASCII, т.к. шрифт мог не загрузиться
        }
        else {
            placeholderText.setFont(m_sfmlFont);
            placeholderText.setString(L"Тут будет рисоваться траектория\n(SFML ver. 2.6.2)"); // Используем L""
        }
        placeholderText.setCharacterSize(16);
        placeholderText.setFillColor(sf::Color(105, 105, 105)); // RGB для DimGray
        sf::FloatRect textRect = placeholderText.getLocalBounds();
        placeholderText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        placeholderText.setPosition(canvasRenderTarget.getSize().x / 2.0f, canvasRenderTarget.getSize().y / 2.0f);
        canvasRenderTarget.draw(placeholderText);
    }
}

void UserInterface::populateTable(const std::vector<TableRowData>& data) {
    if (!m_tableDataGrid) { std::cerr << "Error: m_tableDataGrid is null in populateTable!" << std::endl; return; }
    m_tableDataGrid->removeAllWidgets();

    if (data.empty()) {
        auto emptyLabel = tgui::Label::create(L"Нет данных для отображения"); // L""
        if (emptyLabel) {
            emptyLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
            m_tableDataGrid->addWidget(emptyLabel, 0, 0);
            // Чтобы emptyLabel занимал всю ширину, он должен быть добавлен не в грид,
            // а в сам m_tableDataPanel, если грид пуст.
            // Либо, если он в гриде, грид должен быть настроен на 1 колонку в этом случае.
            // Простейший вариант - он будет в первой колонке.
        }
        m_tableDataPanel->setContentSize({ 0,0 }); // Обновляем размер содержимого для ScrollablePanel
        return;
    }

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& rowData = data[i];

        std::stringstream ss_h, ss_x, ss_y, ss_vx, ss_vy;
        ss_h << std::fixed << std::setprecision(2) << rowData.h_sec;
        ss_x << std::fixed << std::setprecision(2) << rowData.x;
        ss_y << std::fixed << std::setprecision(2) << rowData.y;
        ss_vx << std::fixed << std::setprecision(2) << rowData.Vx;
        ss_vy << std::fixed << std::setprecision(2) << rowData.Vy;

        // Используем sf::String для tgui::Label::create, если передаем литералы с L""
        // или tgui::String если собираем из std::string
        std::vector<tgui::String> rowStrings = {
            tgui::String(ss_h.str()), tgui::String(ss_x.str()), tgui::String(ss_y.str()),
            tgui::String(ss_vx.str()), tgui::String(ss_vy.str())
        };

        for (size_t j = 0; j < rowStrings.size(); ++j) {
            auto cellLabel = tgui::Label::create(rowStrings[j]);
            if (cellLabel) {
                cellLabel->getRenderer()->setTextColor(tgui::Color::Black);
                cellLabel->setHorizontalAlignment(tgui::Label::HorizontalAlignment::Center);
                // cellLabel->getRenderer()->setBorders({0,0,0,1}); // Можно убрать, если линии между ячейками не нужны
                // cellLabel->getRenderer()->setBorderColor(tgui::Color(200, 200, 200));
                m_tableDataGrid->addWidget(cellLabel, i, j);
                m_tableDataGrid->setWidgetPadding(i, j, { 2, 5, 2, 5 }); // T,R,B,L - отступы внутри ячейки
            }
        }
    }
    // Обновляем размер содержимого для ScrollablePanel после добавления всех элементов
    if (m_tableDataPanel && m_tableDataGrid) {
        m_tableDataPanel->setContentSize(m_tableDataGrid->getSize());
    }
}

// --- Главный цикл и обработка событий ---
void UserInterface::run() {
    m_window.setFramerateLimit(60); // Ограничение FPS для плавности и снижения нагрузки
    while (m_window.isOpen()) {
        handleEvents();
        update();
        render();
    }
}

void UserInterface::handleEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        m_gui.handleEvent(event);
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        // Здесь можно добавить обработку событий SFML, если они нужны (например, для Canvas)
    }
}

void UserInterface::update() {
    // Например, анимация или другие обновления состояния, не связанные с вводом пользователя
}

void UserInterface::render() {
    // Отрисовка на Canvas
    if (m_trajectoryCanvas) {
        sf::RenderTexture& canvasRT = m_trajectoryCanvas->getRenderTexture();
        canvasRT.clear(sf::Color(245, 245, 250)); // Светлый фон для канваса
        drawTrajectoryOnCanvas(canvasRT);
        m_trajectoryCanvas->display();
    }

    // Отрисовка основного окна
    m_window.clear(sf::Color(200, 200, 200));
    m_gui.draw();
    m_window.display();
}