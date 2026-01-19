#include "pipelineoptimizer.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

// Основной метод расчета оптимальных параметров трубопровода
QVector<ValidationResult> PipelineOptimizer::calculate(const PipelineParameters& params)
{
    QVector<ValidationResult> results;          // Вектор для хранения результатов расчета
    results.reserve(params.outerDiameters.size()); // Резервирование памяти для эффективности

    // === РАСЧЕТ РАСЧЕТНЫХ СОПРОТИВЛЕНИЙ ПО ТЕКУЧЕСТИ И ПРОЧНОСТИ ===

    // R1 - расчетное сопротивление по текучести (формула из СНиП/СП)
    double R1 = (params.operationalFactor * params.yieldStrength) /
                (params.reliabilityYield * params.responsibilityFactor);

    // R2 - расчетное сопротивление по прочности (формула из СНиП/СП)
    double R2 = (params.operationalFactor * params.tensileStrength) /
                (params.reliabilityStrength * params.responsibilityFactor);

    // Допускаемое эквивалентное напряжение (f_eq = 0.9 по СП 36.13330)
    double allowEquiv = 0.9 * params.yieldStrength;

    qDebug() << "calculate: R1 =" << R1 << ", R2 =" << R2 << ", allowEquiv =" << allowEquiv;

    // === ЦИКЛ ПЕРЕБОРА КАЖДОГО ДИАМЕТРА ИЗ СОРТАМЕНТА ===
    for (double Di : params.outerDiameters) {
        qDebug() << "calculate: Обработка диаметра:" << Di;

        // Создаем структуру результата для текущего диаметра
        ValidationResult res;
        res.diameter = Di;    // Наружный диаметр в мм
        res.isOptimal = false; // Пока не оптимальный
        res.isValid = false;   // Пока не валидный

        // === ПРЕОБРАЗОВАНИЕ ЕДИНИЦ И БАЗОВАЯ ВАЛИДАЦИЯ ===

        double Di_m = Di / 1000.0; // Преобразование мм → м для расчетов

        // Проверка базовых значений на корректность
        if (Di_m <= 0 || params.massFlow <= 0 || params.density <= 0) {
            results.append(res);  // Добавляем пустой результат и переходим к следующему диаметру
            continue;
        }

        // === РАСЧЕТ НАЧАЛЬНОЙ ТОЛЩИНЫ СТЕНКИ (ФОРМУЛА 9) ===

        // δ = (y_fp * p * D) / (2 * min(R1, R2))
        // Определяет минимальную толщину стенки из условия прочности
        double delta = (params.pressureReliability * params.pressure * Di_m) /
                       (2.0 * qMin(R1, R2));
        double initial_delta = delta; // Сохраняем начальное значение для возможного использования

        bool foundValid = false; // Флаг: найден ли подходящий вариант для этого диаметра

        // === ЦИКЛ ПОДБОРА ТОЛЩИНЫ СТЕНКИ ДЛЯ ТЕКУЩЕГО ДИАМЕТРА ===
        while (true) {
            // Проверка толщины стенки на физическую реализуемость
            if (delta <= 0) {
                break; // Некорректная толщина - прерываем цикл по толщине
            }
            if (delta >= Di_m / 2.0) {
                break; // Толщина превышает радиус трубы - физически невозможно
            }

            // === РАСЧЕТ ВНУТРЕННЕГО ДИАМЕТРА (ФОРМУЛА 8) ===

            // d = D - 2δ (где D - наружный диаметр, δ - толщина стенки)
            double di = Di_m - 2.0 * delta;
            if (di <= 0) {
                break; // Внутренний диаметр отрицательный - физически невозможно
            }

            // === РАСЧЕТ СКОРОСТИ ПОТОКА (ФОРМУЛА 1) ===

            // ϑ = 4G / (ρ * π * d²)
            // G - массовый расход, ρ - плотность, d - внутренний диаметр
            double theta = (4.0 * params.massFlow) /
                           (params.density * M_PI * di * di);

            // Проверка на числовую корректность
            if (std::isnan(theta) || std::isinf(theta)) {
                break; // Числовая ошибка - прерываем цикл
            }

            // Сохраняем скорость потока и проверяем допустимый диапазон (1-3 м/с)
            res.flowSpeed = theta;
            res.satisfiesFlowSpeed = (theta >= 1.0 && theta <= 3.0);

            // Если скорость не попадает в допустимый диапазон
            if (!res.satisfiesFlowSpeed) {
                results.append(res);  // Добавляем результат (с пометкой невалидный)
                break; // Прерываем цикл по толщине - нет смысла проверять другие толщины
            }

            // === РАСЧЕТ ПАРАМЕТРОВ ГИДРОУДАРА (ГУ) ===

            // Скорость распространения ударной волны (формула 4):
            // c = 1 / √(ρ/E₀ + d/(E*δ))
            double waveSpeed_val = 1.0 / std::sqrt(
                                       params.density / params.fluidBulkModulus +
                                       di / (params.steelYoungModulus * delta)
                                       );

            if (std::isnan(waveSpeed_val) || std::isinf(waveSpeed_val)) {
                break; // Числовая ошибка
            }

            // Приращение давления при гидроударе (формула 3):
            // Δp = ρ * c * ϑ (в Па, затем конвертируем в МПа)
            double pressureSurge_val = params.density * waveSpeed_val * theta / 1000000.0;

            if (std::isnan(pressureSurge_val) || std::isinf(pressureSurge_val)) {
                qDebug() << "  ОШИБКА: Давление ГУ - NaN или Inf, pressureSurge_val =" << pressureSurge_val;
                break;
            }

            // Полное давление при гидроударе (рабочее + приращение)
            double pressureAtSurge_val = params.pressure + pressureSurge_val;

            if (std::isnan(pressureAtSurge_val) || std::isinf(pressureAtSurge_val)) {
                break;
            }

            // === РАСЧЕТ КОЛЬЦЕВЫХ НАПРЯЖЕНИЙ (ФОРМУЛА 10) ===

            // σ_кц = (y_fp * p_гуд * D) / (2δ)
            // Напряжение от давления с учетом гидроудара
            double hoop = (params.pressureReliability * pressureAtSurge_val * Di_m) /
                          (2.0 * delta);

            if (std::isnan(hoop) || std::isinf(hoop)) {
                break;
            }

            // === РАСЧЕТ ОСЕВЫХ НАПРЯЖЕНИЙ (ФОРМУЛА 14) ===

            // Составляющие осевого напряжения:
            double thermalTerm = -params.steelYoungModulus *
                                 params.thermalExpansionCoeff *
                                 params.temperatureDelta; // Термическая составляющая

            double bendTerm = 0.0; // Изгибная составляющая
            if (params.bendRadius > 0) {
                // Влияние изгиба трубы: σ_изг = E * D / (2r)
                bendTerm = (params.steelYoungModulus * Di_m) / (2.0 * params.bendRadius);
            }

            // Два варианта: "+" и "-" (наиболее опасный случай)
            double axialPlus = params.poissonRatio * hoop + thermalTerm + bendTerm;
            double axialMinus = params.poissonRatio * hoop + thermalTerm - bendTerm;

            // Берем максимальное по абсолютной величине (наиболее опасное)
            double axial = std::abs(axialPlus) > std::abs(axialMinus) ? axialPlus : axialMinus;

            if (std::isnan(axial) || std::isinf(axial)) {
                break;
            }

            // === РАСЧЕТ ЭКВИВАЛЕНТНЫХ НАПРЯЖЕНИЙ (ФОРМУЛА 15) ===

            // σ_экв = √(σ_кц² - σ_кц * σ_пр + σ_пр²) (по теории Мизеса-Генки)
            double equiv = std::sqrt(hoop * hoop - hoop * axial + axial * axial);

            if (std::isnan(equiv) || std::isinf(equiv)) {
                break;
            }

            // === ПРОВЕРКА УСЛОВИЙ ПРОЧНОСТИ ===

            // Проверка по кольцевым напряжениям
            res.satisfiesHoopStress = hoop <= R1;

            // Проверка по осевым напряжениям
            res.satisfiesAxialStress = axial <= R2;

            // Проверка по эквивалентным напряжениям
            res.satisfiesEquivalentStress = equiv <= allowEquiv;

            // === ПРОВЕРКА ВСЕХ УСЛОВИЙ ПРОЧНОСТИ ОДНОВРЕМЕННО ===
            if (res.satisfiesHoopStress &&
                res.satisfiesAxialStress &&
                res.satisfiesEquivalentStress) {

                // РАСЧЕТ КОЭФФИЦИЕНТОВ ЗАПАСА ПРОЧНОСТИ
                // Коэффициент запаса = допускаемое напряжение / фактическое напряжение
                res.safetyHoop = (hoop > 0.0) ? R1 / hoop : 0.0;
                res.safetyAxial = (axial > 0.0) ? R2 / axial : 0.0;
                res.safetyEquivalent = (equiv > 0.0) ? allowEquiv / equiv : 0.0;

                // Сохраняем найденную толщину стенки (в метрах)
                res.finalThickness = delta;

                // Помечаем как оптимальный (пока локально для этого диаметра)
                res.isOptimal = true;
                res.isValid = true;
                foundValid = true; // Флаг, что для этого диаметра найден подходящий вариант

                results.append(res); // Добавляем успешный результат

                // Выходим из цикла по толщине - нашли первую подходящую толщину
                // (алгоритм использует минимально допустимую толщину)
                break;
            } else {
                // Условия прочности не выполнены - УВЕЛИЧИВАЕМ ТОЛЩИНУ СТЕНКИ
                qDebug() << "  Напряжения не подходят. Увеличиваем толщину на 1 мм.";
                delta += 0.001; // Увеличение на 1 мм (0.001 м)

                // Проверяем, не превышает ли толщина физический предел
                if (delta >= Di_m / 2.0) {
                    break; // Толщина превысила радиус - прерываем цикл
                }
                // Продолжаем цикл с увеличенной толщиной
            }
        } // Конец цикла по толщине

        // Если для текущего диаметра не найден подходящий вариант
        if (!foundValid) {
            // Не добавляем результат - пропускаем этот диаметр
        }
    } // Конец цикла по диаметрам

    // === ВЫБОР ОПТИМАЛЬНОГО ДИАМЕТРА СРЕДИ ВСЕХ ПОДХОДЯЩИХ ===

    // Собираем все успешные результаты
    QVector<ValidationResult> validResults;
    for (const auto& res : results) {
        if (res.isOptimal) {
            validResults.append(res);
        }
    }

    if (!validResults.isEmpty()) {
        // Находим диаметр с МАКСИМАЛЬНЫМ минимальным коэффициентом запаса
        // (наиболее надежный вариант)
        auto bestIt = std::max_element(validResults.begin(), validResults.end(),
                                       [](const ValidationResult& a, const ValidationResult& b) {
                                           // Находим минимальный коэффициент запаса для каждого варианта
                                           double minSafetyA = std::min({a.safetyHoop,
                                                                         a.safetyAxial,
                                                                         a.safetyEquivalent});
                                           double minSafetyB = std::min({b.safetyHoop,
                                                                         b.safetyAxial,
                                                                         b.safetyEquivalent});
                                           // Ищем максимальный из минимальных запасов
                                           return minSafetyA < minSafetyB;
                                       });

        // Помечаем только найденный оптимальный диаметр
        for (auto& res : results) {
            // Сравнение с плавающей точкой с заданной точностью
            res.isOptimal = (qFuzzyCompare(res.diameter, bestIt->diameter));
        }

        // Дополнительная информация об оптимальном варианте
        double minSafety = std::min({bestIt->safetyHoop,
                                     bestIt->safetyAxial,
                                     bestIt->safetyEquivalent});
    } else {
        // Если ни один диаметр не подошел - сбрасываем флаги оптимальности
        for (auto& res : results) {
            res.isOptimal = false;
        }
    }

    return results; // Возвращаем все результаты расчета
}
