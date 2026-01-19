#ifndef PIPELINEPARAMETERS_H
#define PIPELINEPARAMETERS_H

#include <QVector>
#include <QRectF>

// Режим работы расчета
enum class Mode {
    Mode1,  // Универсальный расчёт для типовых условий
    Mode2   // Расчёт для индивидуальных условий
};

// Результат валидации трубопровода
struct ValidationResult {
    double diameter;               // Наружный диаметр трубы, мм
    double finalThickness;         // Толщина стенки трубы, мм
    bool satisfiesFlowSpeed = false;     // Удовлетворяет ли условию по скорости потока
    bool satisfiesHoopStress = false;    // Удовлетворяет ли условию по кольцевым напряжениям
    bool satisfiesAxialStress = false;   // Удовлетворяет ли условию по продольным напряжениям
    bool satisfiesEquivalentStress = false; // Удовлетворяет ли условию по эквивалентным напряжениям
    double safetyHoop = 0.0;       // Коэффициент запаса по кольцевым напряжениям (n^{кц})
    double safetyAxial = 0.0;      // Коэффициент запаса по продольным напряжениям (n^{пр})
    double minSafery = 0.0;        // Минимальный коэффициент запаса (наименьший из трех)
    double safetyEquivalent = 0.0; // Коэффициент запаса по эквивалентным напряжениям (n^{экв})
    bool isOptimal = false;        // Является ли данный диаметр оптимальным (D_{опт})
    double flowSpeed = 0.0;        // Скорость потока среды в трубе, м/с
    bool isValid = false;          // Общая валидность результата
};

// Информация о сегменте трубопровода для графического отображения
struct PipeSegmentInfo {
    int segmentIndex;              // Индекс сегмента трубопровода
    double leftX;                  // Левая координата X сегмента
    double rightX;                 // Правая координата X сегмента
    QRectF innerRect;              // Внутренний прямоугольник трубы (для отрисовки)
};

// Основная структура параметров трубопровода
struct PipelineParameters {
    // === ОБЩИЕ ПАРАМЕТРЫ ===

    double pressure;               // p - Эксплуатационное давление, МПа
    double massFlow;               // G - Массовый расход транспортируемого продукта (ТП), кг/с
    double operationalFactor;      // m - Коэффициент условий работы ТП (безразмерный)
    double reliabilityYield;       // y_my - Коэффициент надежности по материалу труб по текучести
    double reliabilityStrength;    // y_mu - Коэффициент надежности по материалу труб по прочности
    double responsibilityFactor;   // y_n - Коэффициент надежности по ответственности ТП
    double pressureReliability;    // y_fp - Коэффициент надежности по внутреннему давлению
    QVector<double> outerDiameters; // D_i - Наружный диаметр ТП в соответствии с сортаментом, мм

    // === ПАРАМЕТРЫ ТОЛЬКО ДЛЯ РЕЖИМА 2 ===

    double density;                // ρ - Плотность перекачиваемой среды, кг/м³
    double yieldStrength;          // σ_т - Предел текучести стали, МПа
    double tensileStrength;        // σ_п - Предел прочности стали, МПа
    double fluidBulkModulus;       // E_0 - Модуль упругости перекачиваемой среды, МПа
    double steelYoungModulus;      // E - Модуль упругости стали, МПа
    double temperatureDelta;       // Δt - Температурный перепад, °C
    double poissonRatio;           // μ - Коэффициент Пуассона (безразмерный)
    double thermalExpansionCoeff;  // α - Коэффициент линейного температурного расширения, 1/°C
    double bendRadius;             // r - Радиус упругого изгиба трубопровода, м

    Mode mode;
};

#endif // PIPELINEPARAMETERS_H
