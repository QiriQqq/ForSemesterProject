#include "TrajectoryVisualizer.h"

// --- Инициализация статических констант (если они объявлены как static в .h) ---
// constexpr float TrajectoryVisualizer::DEFAULT_SCALE; // И т.д. для всех static constexpr
// Но для литеральных типов static constexpr можно инициализировать прямо в .h (C++17+)
// Если компилятор старый, то так:
// const float TrajectoryVisualizer::DEFAULT_SCALE = 150.0f;
// const unsigned int TrajectoryVisualizer::DEFAULT_POINTS_PER_FRAME = 1u; 
// ... и так далее для других ...

TrajectoryVisualizer::TrajectoryVisualizer(unsigned int width, unsigned int height, const std::string& windowTitle)
    : m_window(sf::VideoMode(width, height), windowTitle, sf::Style::Default), // Используем L"" для кириллицы в заголовке, если нужно
    m_scale(DEFAULT_SCALE),
    m_offset(0.f, 0.f),
    m_screenCenter(static_cast<float>(width) / 2.f, static_cast<float>(height) / 2.f),
    m_currentPointIndex(0),
    m_pointsPerFrame(DEFAULT_POINTS_PER_FRAME),
    m_isPaused(false),
    m_showAllPointsImmediately(false),
    m_isDragging(false) {
    m_window.setFramerateLimit(60);
    setupInfoText();
}

void TrajectoryVisualizer::setData(const WorldTrajectoryData& data) {
    m_worldTrajectoryData = data;
    resetViewAndAnimation();
    // recalculateScreenTrajectory(); // Вызывается внутри resetViewAndAnimation
}

bool TrajectoryVisualizer::loadDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "TrajectoryVisualizer: Ошибка: не удалось открыть файл траектории " << filename << "\n";
        return false;
    }
    WorldTrajectoryData data;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double x, y;
        if (iss >> x >> y) {
            data.emplace_back(x, y);
        }
        else {
            std::cerr << "TrajectoryVisualizer: Предупреждение: неверный формат строки в файле: " << line << "\n";
        }
    }
    file.close(); // Закрываем файл
    if (data.empty()) {
        std::cerr << "TrajectoryVisualizer: Ошибка: файл " << filename << " пуст или не содержит корректных данных.\n";
        return false;
    }
    setData(data);
    return true;
}

void TrajectoryVisualizer::run() {
    if (m_worldTrajectoryData.empty()) {
        std::cerr << "TrajectoryVisualizer: Нет данных для визуализации. Загрузите данные.\n";
        // Можно просто показать пустое окно с сообщением
        bool dataNotLoaded = true;
        while (m_window.isOpen() && dataNotLoaded) {
            sf::Event event{};
            while (m_window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) m_window.close();
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) m_window.close();
            }
            updateInfoText(); // Обновит текст, который может содержать сообщение об ошибке
            m_window.clear(sf::Color::Black);
            m_window.draw(m_infoText); // Показать инфо-текст (можно изменить его содержимое)
            m_window.display();
            if (!m_worldTrajectoryData.empty()) dataNotLoaded = false; // Если данные загрузились в другом потоке/способом
        }
        if (!m_window.isOpen()) return; // Если окно было закрыто
    }

    while (m_window.isOpen()) {
        sf::Event event{};
        while (m_window.pollEvent(event)) {
            handleEvent(event);
        }
        updateAnimation();
        updateInfoText();
        draw();
    }
}

void TrajectoryVisualizer::resetViewAndAnimation() {
    m_scale = DEFAULT_SCALE;
    m_offset = { 0.f, 0.f };
    m_isPaused = false;
    m_showAllPointsImmediately = false;
    m_pointsPerFrame = DEFAULT_POINTS_PER_FRAME;
    m_currentPointIndex = m_worldTrajectoryData.empty() ? 0 : 1;
    recalculateScreenTrajectory();
}

sf::Vector2f TrajectoryVisualizer::toScreenCoords(double worldX, double worldY) const {
    return {
        m_screenCenter.x + m_offset.x + static_cast<float>(worldX) * m_scale,
        m_screenCenter.y + m_offset.y - static_cast<float>(worldY) * m_scale
    };
}

sf::Vector2f TrajectoryVisualizer::toWorldCoords(sf::Vector2f screenPos) const {
    return {
        (screenPos.x - m_screenCenter.x - m_offset.x) / m_scale,
       -(screenPos.y - m_screenCenter.y - m_offset.y) / m_scale
    };
}

void TrajectoryVisualizer::recalculateScreenTrajectory() {
    m_screenTrajectory.clear();
    if (m_worldTrajectoryData.empty()) return;

    m_screenTrajectory.reserve(m_worldTrajectoryData.size());
    for (const auto& world_point : m_worldTrajectoryData) {
        m_screenTrajectory.emplace_back(toScreenCoords(world_point.first, world_point.second), sf::Color::White);
    }

    if (!m_showAllPointsImmediately) {
        m_currentPointIndex = std::min(m_currentPointIndex, m_screenTrajectory.size());
        if (m_currentPointIndex == 0 && !m_screenTrajectory.empty()) {
            m_currentPointIndex = 1;
        }
    }
    else {
        m_currentPointIndex = m_screenTrajectory.size();
    }
}

void TrajectoryVisualizer::setupInfoText() {
    if (!m_font.loadFromFile(FONT_FILENAME)) {
        std::cerr << "TrajectoryVisualizer: Ошибка: не удалось загрузить шрифт " << FONT_FILENAME << "\n";
    }
    m_infoText.setFont(m_font);
    m_infoText.setCharacterSize(INFO_TEXT_CHAR_SIZE);
    m_infoText.setFillColor(sf::Color::Yellow);
    m_infoText.setPosition(10.f, 10.f);
}

void TrajectoryVisualizer::updateInfoText() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Scale: " << m_scale << "\n";
    oss << "Offset: (" << m_offset.x << ", " << m_offset.y << ")\n";
    oss << "Points drawn: " << m_currentPointIndex << "/" << m_worldTrajectoryData.size() << "\n";
    oss << "Animation: " << (m_isPaused ? "Paused" : "Running")
        << " (" << m_pointsPerFrame << " pts/frame)\n";
    oss << "Controls:\n";
    oss << "  Mouse Wheel: Zoom\n";
    oss << "  Right Mouse Drag: Pan\n";
    oss << "  P: Pause/Resume animation\n";
    oss << "  F: Toggle full trajectory\n";
    oss << "  +/-: Change animation speed\n";
    oss << "  R: Reset view & animation\n";
    oss << "  Esc: Exit";
    m_infoText.setString(oss.str()); // Для sf::Text лучше использовать sf::String или L"" если есть кириллица
    // но здесь только ASCII, так что oss.str() должен работать.
    // Для надежности можно: m_infoText.setString(sf::String::fromUtf8(oss.str().c_str()));
}

void TrajectoryVisualizer::handleEvent(const sf::Event& event) {
    switch (event.type) {
    case sf::Event::Closed:
        m_window.close();
        break;
    case sf::Event::Resized:
    {
        sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
        m_window.setView(sf::View(visibleArea));
        m_screenCenter = { event.size.width / 2.f, event.size.height / 2.f };
        recalculateScreenTrajectory();
    }
    break;
    case sf::Event::KeyPressed:
        handleKeyPress(event.key);
        break;
    case sf::Event::MouseWheelScrolled:
        if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel && event.mouseWheelScroll.delta != 0) { // Проверяем тип колеса
            sf::Vector2f worldPosBeforeZoom = toWorldCoords(static_cast<sf::Vector2f>(sf::Mouse::getPosition(m_window)));
            float zoomFactor = (event.mouseWheelScroll.delta > 0) ? ZOOM_FACTOR_STEP : 1.0f / ZOOM_FACTOR_STEP;
            m_scale *= zoomFactor;
            sf::Vector2f worldPosAfterZoom = toWorldCoords(static_cast<sf::Vector2f>(sf::Mouse::getPosition(m_window)));
            m_offset.x += (worldPosAfterZoom.x - worldPosBeforeZoom.x) * m_scale;
            m_offset.y += (worldPosAfterZoom.y - worldPosBeforeZoom.y) * m_scale;
            recalculateScreenTrajectory();
        }
        break;
    case sf::Event::MouseButtonPressed:
        if (event.mouseButton.button == sf::Mouse::Right) {
            m_isDragging = true;
            m_lastMousePos = sf::Mouse::getPosition(m_window);
        }
        break;
    case sf::Event::MouseButtonReleased:
        if (event.mouseButton.button == sf::Mouse::Right) {
            m_isDragging = false;
        }
        break;
    case sf::Event::MouseMoved:
        if (m_isDragging) {
            sf::Vector2i newMousePos = sf::Mouse::getPosition(m_window);
            sf::Vector2f delta = static_cast<sf::Vector2f>(newMousePos - m_lastMousePos);
            m_offset += delta;
            m_lastMousePos = newMousePos;
            recalculateScreenTrajectory();
        }
        break;
    default:
        break;
    }
}

void TrajectoryVisualizer::handleKeyPress(const sf::Event::KeyEvent& keyEvent) {
    if (keyEvent.code == sf::Keyboard::Escape) m_window.close();
    if (keyEvent.code == sf::Keyboard::P) m_isPaused = !m_isPaused;
    if (keyEvent.code == sf::Keyboard::F) {
        m_showAllPointsImmediately = !m_showAllPointsImmediately;
        if (m_showAllPointsImmediately) {
            m_currentPointIndex = m_screenTrajectory.size();
        }
        else {
            m_currentPointIndex = m_screenTrajectory.empty() ? 0 : 1;
        }
    }
    if (keyEvent.code == sf::Keyboard::Add || keyEvent.code == sf::Keyboard::Equal) { // Equal это + на основной клавиатуре
        m_pointsPerFrame = std::min(m_pointsPerFrame * ANIMATION_SPEED_MULTIPLIER, MAX_POINTS_PER_FRAME);
    }
    if (keyEvent.code == sf::Keyboard::Subtract || keyEvent.code == sf::Keyboard::Hyphen) { // Hyphen это - на основной клавиатуре
        m_pointsPerFrame = std::max(m_pointsPerFrame / ANIMATION_SPEED_MULTIPLIER, MIN_POINTS_PER_FRAME);
    }
    if (keyEvent.code == sf::Keyboard::R) resetViewAndAnimation();
}

void TrajectoryVisualizer::updateAnimation() {
    if (!m_isPaused && !m_showAllPointsImmediately && m_currentPointIndex < m_screenTrajectory.size()) {
        m_currentPointIndex = std::min(m_screenTrajectory.size(), m_currentPointIndex + m_pointsPerFrame);
    }
}

void TrajectoryVisualizer::draw() {
    m_window.clear(sf::Color::Black);

    sf::CircleShape centerMassShape(CENTER_POINT_RADIUS);
    centerMassShape.setFillColor(sf::Color::Red);
    centerMassShape.setOrigin(CENTER_POINT_RADIUS, CENTER_POINT_RADIUS);
    centerMassShape.setPosition(toScreenCoords(0, 0));
    m_window.draw(centerMassShape);

    if (!m_screenTrajectory.empty()) {
        size_t pointsToDraw = std::min(m_currentPointIndex, m_screenTrajectory.size());
        if (pointsToDraw >= 2) {
            m_window.draw(&m_screenTrajectory[0], pointsToDraw, sf::LineStrip);
        }
        else if (pointsToDraw == 1) {
            sf::CircleShape firstPointShape(TRAJECTORY_START_POINT_RADIUS);
            firstPointShape.setFillColor(sf::Color::White);
            firstPointShape.setOrigin(TRAJECTORY_START_POINT_RADIUS, TRAJECTORY_START_POINT_RADIUS);
            firstPointShape.setPosition(m_screenTrajectory[0].position);
            m_window.draw(firstPointShape);
        }
    }

    m_window.draw(m_infoText);
    m_window.display();
}

bool TrajectoryVisualizer::saveTrajectoryToFile(const std::string& filename) const {
    if (m_worldTrajectoryData.empty()) {
        std::cerr << "TrajectoryVisualizer: Нет данных траектории для сохранения в файл '" << filename << "'.\n";
        return false; // Возвращаем false, если данных нет
    }

    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "TrajectoryVisualizer: Ошибка: не удалось открыть файл '" << filename << "' для записи.\n";
        return false;
    }

    // Устанавливаем форматирование для вывода чисел с плавающей точкой
    // для лучшей читаемости и точности в файле
    outputFile << std::fixed << std::setprecision(10); // 10 знаков после запятой

    for (const auto& point : m_worldTrajectoryData) {
        // m_worldTrajectoryData это std::vector<std::pair<double, double>>
        // point.first это x, point.second это y
        outputFile << point.first << " " << point.second << "\n";
    }
        
    outputFile.close(); // Закрываем файл

    if (outputFile.fail()) { // Проверяем, не возникло ли ошибок при записи или закрытии
        std::cerr << "TrajectoryVisualizer: Ошибка при записи или закрытии файла '" << filename << "'.\n";
        return false;
    }

    std::cout << "TrajectoryVisualizer: Траектория (" << m_worldTrajectoryData.size()
        << " точек) успешно сохранена в файл '" << filename << "'.\n";
    return true;
}