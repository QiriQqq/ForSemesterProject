﻿#include "Calculations.h"         // Для расчетов
#include "TrajectoryVisualizer.h" // Для визуализации
#include "UserInterface.h"        // Для вашего TGUI интерфейса

#include <iostream>
#include <string>
#include <stdexcept>   // Для tgui::Exception и std::exception
#include <iomanip>     // Для std::fixed, std::setprecision в saveTrajectoryToFile
#include <fstream>     // Для std::ofstream в saveTrajectoryToFile

void saveTrajectoryToFile(const WorldTrajectoryData& trajectoryData, const std::string& filename);


int main() {
    setlocale(LC_ALL, "Rus");

    // 1. ОКНО ПРОГРАММЫ //

    try {
        UserInterface uiApp;
        uiApp.run();
    }
    catch (const tgui::Exception& e) {
        std::cerr << "TGUI Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Runtime Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "An unknown C++ exception occurred." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


void saveTrajectoryToFile(const WorldTrajectoryData& trajectoryData, const std::string& filename) {
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл '" << filename << "' для записи.\n";
        return;
    }
    // Используем высокую точность для сохранения данных
    fout << std::fixed << std::setprecision(10);
    for (const auto& point : trajectoryData) {
        fout << point.first << " " << point.second << "\n";
    }
    fout.close();
    std::cout << "Результаты симуляции (" << trajectoryData.size() << " точек) записаны в " << filename << "\n";
}