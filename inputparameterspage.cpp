#include "inputparameterspage.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <stdexcept>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QMessageBox>

// Конструктор класса страницы ввода параметров
InputParametersPage::InputParametersPage(QWidget *parent)
    : QWidget(parent)
    , m_mode(Mode::Mode1)           // Установка режима по умолчанию (упрощенный)
{
    setupForm();                    // Вызов метода создания и настройки формы
}

// Метод преобразования введенных данных в структуру параметров
PipelineParameters InputParametersPage::toParameters() const
{
    // --- ДОБАВИТЬ ЭТИ ПРОВЕРКИ ---
    if (!m_pressure || !m_massFlow || !m_operationalFactor ||
        !m_reliabilityYield || !m_reliabilityStrength ||
        !m_responsibilityFactor || !m_pressureReliability ||
        !m_outerDiameters) {
        throw std::runtime_error("InputParametersPage: один или несколько виджетов не инициализированы.");
    }

    if (m_mode == Mode::Mode2) {
        if (!m_density || !m_yieldStrength || !m_tensileStrength ||
            !m_fluidBulkModulus || !m_steelYoungModulus ||
            !m_temperatureDelta || !m_poissonRatio ||
            !m_thermalExpansionCoeff || !m_bendRadius) {
            throw std::runtime_error("InputParametersPage: один или несколько виджетов Mode2 не инициализированы.");
        }
    }

    PipelineParameters p;          // Создание экземпляра структуры параметров
    p.mode = m_mode;

    // === ОБЩИЕ ПАРАМЕТРЫ ===

    // Получение значений из виджетов ввода и присвоение их полям структуры
    p.pressure = m_pressure->value();
    p.massFlow = m_massFlow->value();
    p.operationalFactor = m_operationalFactor->value();
    p.reliabilityYield = m_reliabilityYield->value();
    p.reliabilityStrength = m_reliabilityStrength->value();
    p.responsibilityFactor = m_responsibilityFactor->value();
    p.pressureReliability = m_pressureReliability->value();
    p.outerDiameters = outerDiameters();

    // ВАЛИДАЦИЯ ВВЕДЕННЫХ ДИАМЕТРОВ
    // Проверка: введен ли хотя бы один диаметр
    if (p.outerDiameters.isEmpty()) {
        throw std::invalid_argument("Введите хотя бы один наружный диаметр.");
    }

    // Проверка каждого диаметра на соответствие допустимому диапазону (100-1400 мм)
    for (double d : p.outerDiameters) {
        if ((d < 100.0) || (d > 1400.0)) {
            throw std::invalid_argument(
                QString("Диаметр %1 мм вне допустимого диапазона (100–1400 мм).").arg(d).toStdString());
        }
    }

    // === ПАРАМЕТРЫ В ЗАВИСИМОСТИ ОТ РЕЖИМА ===
    if (m_mode == Mode::Mode2) {
        // РЕЖИМ 2: все параметры берутся из полей ввода

        p.density = m_density->value();
        p.yieldStrength = m_yieldStrength->value();
        p.tensileStrength = m_tensileStrength->value();
        p.fluidBulkModulus = m_fluidBulkModulus->value();
        p.steelYoungModulus = m_steelYoungModulus->value();
        p.temperatureDelta = m_temperatureDelta->value();
        p.poissonRatio = m_poissonRatio->value();
        p.thermalExpansionCoeff = m_thermalExpansionCoeff->value();
        p.bendRadius = m_bendRadius->value();
    } else {
        // РЕЖИМ 1: используются типовые значения

        p.density = 850.0;
        p.yieldStrength = 343.0;
        p.tensileStrength = 490.0;
        p.fluidBulkModulus = 1300.0;
        p.steelYoungModulus = 200000.0;
        p.temperatureDelta = 20.0;
        p.poissonRatio = 0.3;
        p.thermalExpansionCoeff = 11.4e-6;
        p.bendRadius = 0.0;
    }

    return p;
}

// Метод переключения режима работы (Mode1/Mode2)
void InputParametersPage::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;

        bool showMode2Fields = (mode == Mode::Mode2);

        // --- ДОБАВИТЬ ПРОВЕРКИ ---
        if (m_density) m_density->setVisible(showMode2Fields);
        if (m_yieldStrength) m_yieldStrength->setVisible(showMode2Fields);
        if (m_tensileStrength) m_tensileStrength->setVisible(showMode2Fields);
        if (m_fluidBulkModulus) m_fluidBulkModulus->setVisible(showMode2Fields);
        if (m_steelYoungModulus) m_steelYoungModulus->setVisible(showMode2Fields);
        if (m_temperatureDelta) m_temperatureDelta->setVisible(showMode2Fields);
        if (m_poissonRatio) m_poissonRatio->setVisible(showMode2Fields);
        if (m_thermalExpansionCoeff) m_thermalExpansionCoeff->setVisible(showMode2Fields);
        if (m_bendRadius) m_bendRadius->setVisible(showMode2Fields);
    }
}
// Метод создания и настройки пользовательского интерфейса
void InputParametersPage::setupForm()
{
    // === СОЗДАНИЕ ОСНОВНОЙ КОМПОНОВКИ ===

    // Вертикальная компоновка для всего виджета
    m_mainLayout = new QVBoxLayout(this);
    setLayout(m_mainLayout);

    // Форма для полей ввода (метка + поле)
    m_formLayout = new QFormLayout();
    m_mainLayout->addLayout(m_formLayout);

    // === СОЗДАНИЕ И НАСТРОЙКА ВИДЖЕТОВ ДЛЯ ОБЩИХ ПАРАМЕТРОВ ===

    // Виджет для ввода давления с валидацией диапазона
    m_pressure = new QDoubleSpinBox();
    m_pressure->setRange(0.1, 20.0);     // Допустимый диапазон: 0.1-20.0 МПа
    m_pressure->setDecimals(2);          // Два знака после запятой
    m_pressure->setSuffix(" МПа");       // Единицы измерения
    m_pressure->setValue(10.0);          // Значение по умолчанию

    // Виджет для массового расхода
    m_massFlow = new QDoubleSpinBox();
    m_massFlow->setRange(1.0, 10000.0);  // Диапазон 1-10000 кг/с
    m_massFlow->setDecimals(2);
    m_massFlow->setSuffix(" кг/с");
    m_massFlow->setValue(50.0);

    // Виджет для коэффициента условий работы
    m_operationalFactor = new QDoubleSpinBox();
    m_operationalFactor->setRange(0.1, 2.0);
    m_operationalFactor->setDecimals(3);
    m_operationalFactor->setValue(1.0);  // Нейтральное значение

    // Виджеты для различных коэффициентов надежности
    m_reliabilityYield = new QDoubleSpinBox();
    m_reliabilityYield->setRange(0.1, 2.0);
    m_reliabilityYield->setDecimals(3);
    m_reliabilityYield->setValue(1.0);

    m_reliabilityStrength = new QDoubleSpinBox();
    m_reliabilityStrength->setRange(0.1, 2.0);
    m_reliabilityStrength->setDecimals(3);
    m_reliabilityStrength->setValue(1.0);

    m_responsibilityFactor = new QDoubleSpinBox();
    m_responsibilityFactor->setRange(0.1, 2.0);
    m_responsibilityFactor->setDecimals(3);
    m_responsibilityFactor->setValue(1.0);

    m_pressureReliability = new QDoubleSpinBox();
    m_pressureReliability->setRange(0.1, 2.0);
    m_pressureReliability->setDecimals(3);
    m_pressureReliability->setValue(1.0);

    // Текстовое поле для ввода диаметров
    m_outerDiameters = new QLineEdit();
    m_outerDiameters->setPlaceholderText("Например: 530, 720, 820");
    // Валидатор: только цифры, пробелы и запятые
    m_outerDiameters->setValidator(new QRegularExpressionValidator(
        QRegularExpression("^[\\d\\s,]+$"), this));

    // Добавление виджетов в форму
    m_formLayout->addRow("Эксплуатационное давление:", m_pressure);
    m_formLayout->addRow("Массовый расход:", m_massFlow);
    m_formLayout->addRow("Коэффициент условий работы (m):", m_operationalFactor);
    m_formLayout->addRow("Коэффициент надёжности по текучести (y_my):", m_reliabilityYield);
    m_formLayout->addRow("Коэффициент надёжности по прочности (y_mu):", m_reliabilityStrength);
    m_formLayout->addRow("Коэффициент надёжности по ответственности (y_n):", m_responsibilityFactor);
    m_formLayout->addRow("Коэффициент надёжности по давлению (y_fp):", m_pressureReliability);
    m_formLayout->addRow("Наружные диаметры (мм, через запятую):", m_outerDiameters);

    // === СОЗДАНИЕ ВИДЖЕТОВ ДЛЯ ПАРАМЕТРОВ РЕЖИМА 2 ===
    // Виджеты создаются сразу, но скрываются по умолчанию

    // Плотность перекачиваемой среды
    m_density = new QDoubleSpinBox();
    m_density->setRange(700.0, 1000.0);
    m_density->setDecimals(1);
    m_density->setSuffix(" кг/м³");
    m_density->setValue(850.0);

    // Механические свойства стали
    m_yieldStrength = new QDoubleSpinBox();
    m_yieldStrength->setRange(200.0, 1000.0);
    m_yieldStrength->setDecimals(1);
    m_yieldStrength->setSuffix(" МПа");
    m_yieldStrength->setValue(600.0);

    m_tensileStrength = new QDoubleSpinBox();
    m_tensileStrength->setRange(200.0, 1000.0);
    m_tensileStrength->setDecimals(1);
    m_tensileStrength->setSuffix(" МПа");
    m_tensileStrength->setValue(600.0);

    // Модули упругости
    m_fluidBulkModulus = new QDoubleSpinBox();
    m_fluidBulkModulus->setRange(1000.0, 250000.0);
    m_fluidBulkModulus->setSuffix(" МПа");
    m_fluidBulkModulus->setValue(3000.0);

    m_steelYoungModulus = new QDoubleSpinBox();
    m_steelYoungModulus->setRange(1000.0, 250000.0);
    m_steelYoungModulus->setSuffix(" МПа");
    m_steelYoungModulus->setValue(3000.0);

    // Температурные параметры
    m_temperatureDelta = new QDoubleSpinBox();
    m_temperatureDelta->setRange(-60.0, 60.0);
    m_temperatureDelta->setDecimals(1);
    m_temperatureDelta->setSuffix(" °C");

    // Геометрические и физические параметры
    m_poissonRatio = new QDoubleSpinBox();
    m_poissonRatio->setRange(0.2, 0.4);
    m_poissonRatio->setDecimals(3);

    m_thermalExpansionCoeff = new QDoubleSpinBox();
    m_thermalExpansionCoeff->setRange(1e-6, 20e-6);
    m_thermalExpansionCoeff->setDecimals(8);
    m_thermalExpansionCoeff->setSuffix(" 1/°C");

    m_bendRadius = new QDoubleSpinBox();
    m_bendRadius->setRange(0.0, 10000.0);  // 0 - прямая труба
    m_bendRadius->setDecimals(1);
    m_bendRadius->setSuffix(" м");

    // Добавление виджетов Mode2 в форму
    m_formLayout->addRow("Плотность:", m_density);
    m_formLayout->addRow("Предел текучести:", m_yieldStrength);
    m_formLayout->addRow("Предел прочности:", m_tensileStrength);
    m_formLayout->addRow("Модуль упругости среды:", m_fluidBulkModulus);
    m_formLayout->addRow("Модуль упругости стали:", m_steelYoungModulus);
    m_formLayout->addRow("Температурный перепад:", m_temperatureDelta);
    m_formLayout->addRow("Коэффициент Пуассона:", m_poissonRatio);
    m_formLayout->addRow("Коэфф. лин. расширения:", m_thermalExpansionCoeff);
    m_formLayout->addRow("Радиус изгиба:", m_bendRadius);

    // СКРЫТИЕ ПОЛЕЙ MODE2 ПО УМОЛЧАНИЮ (так как начальный режим - Mode1)
    m_density->setVisible(false);
    m_yieldStrength->setVisible(false);
    m_tensileStrength->setVisible(false);
    m_fluidBulkModulus->setVisible(false);
    m_steelYoungModulus->setVisible(false);
    m_temperatureDelta->setVisible(false);
    m_poissonRatio->setVisible(false);
    m_thermalExpansionCoeff->setVisible(false);
    m_bendRadius->setVisible(false);

    // === СОЗДАНИЕ ПАНЕЛИ КНОПОК УПРАВЛЕНИЯ ===

    auto buttonLayout = new QHBoxLayout();  // Горизонтальная компоновка для кнопок
    auto nextBtn = new QPushButton("Рассчитать");
    auto backBtn = new QPushButton("Назад");
    buttonLayout->addWidget(backBtn);
    buttonLayout->addWidget(nextBtn);

    // ПОДКЛЮЧЕНИЕ СИГНАЛОВ КНОПОК К СЛОТАМ
    connect(nextBtn, &QPushButton::clicked, this, &InputParametersPage::onNextClicked);
    connect(backBtn, &QPushButton::clicked, this, &InputParametersPage::onBackClicked);

    m_mainLayout->addLayout(buttonLayout);  // Добавление панели кнопок в основную компоновку

    // Установка минимального размера окна
    setMinimumSize(600, 400);
}

// Метод преобразования текста из поля диаметров в вектор чисел
QVector<double> InputParametersPage::outerDiameters() const
{
    QString text = m_outerDiameters->text().trimmed();  // Получение и очистка текста
    if (text.isEmpty()) {
        return {};  // Возврат пустого вектора, если текст пустой
    }

    QVector<double> diameters;  // Вектор для хранения результата

    // Разделение строки по запятым
    QStringList parts = text.split(',', Qt::SkipEmptyParts);

    // Обработка каждой части
    for (const QString& part : parts) {
        QString cleanPart = part.trimmed();  // Удаление пробелов
        if (cleanPart.isEmpty()) continue;   // Пропуск пустых строк

        bool ok;
        double d = cleanPart.toDouble(&ok);  // Преобразование в число

        // Добавление в вектор, если преобразование успешно и значение положительное
        if (ok && d > 0) {
            diameters.append(d);
        }
    }

    return diameters;  // Возврат вектора диаметров
}

// СЛОТ: обработка нажатия кнопки "Рассчитать"
void InputParametersPage::onNextClicked()
{
    // Проверка количества введенных диаметров перед расчетом
    QVector<double> diameters = outerDiameters();

    // Ограничение: не более 10 диаметров для производительности
    if (diameters.size() > 10) {
        QMessageBox::warning(this, "Слишком много диаметров",
                             QString("Вы ввели %1 диаметров. Максимально допустимое количество - 10.\n"
                                     "Пожалуйста, удалите лишние значения.").arg(diameters.size()));
        return;  // Прерывание обработки
    }

    // Если проверка пройдена - отправляем сигнал для перехода к расчету
    emit nextRequested();
}

// СЛОТ: обработка нажатия кнопки "Назад"
void InputParametersPage::onBackClicked()
{
    emit backRequested();  // Сигнал для возврата на предыдущую страницу
}

// Метод очистки всех полей ввода
void InputParametersPage::clearFields()
{
    // --- ДОБАВИТЬ ПРОВЕРКИ ---
    if (m_pressure) m_pressure->clear();
    if (m_massFlow) m_massFlow->clear();
    if (m_operationalFactor) m_operationalFactor->clear();
    if (m_reliabilityYield) m_reliabilityYield->clear();
    if (m_reliabilityStrength) m_reliabilityStrength->clear();
    if (m_responsibilityFactor) m_responsibilityFactor->clear();
    if (m_pressureReliability) m_pressureReliability->clear();
    if (m_outerDiameters) m_outerDiameters->clear();

    if (m_density) m_density->clear();
    if (m_yieldStrength) m_yieldStrength->clear();
    if (m_tensileStrength) m_tensileStrength->clear();
    if (m_fluidBulkModulus) m_fluidBulkModulus->clear();
    if (m_steelYoungModulus) m_steelYoungModulus->clear();
    if (m_temperatureDelta) m_temperatureDelta->clear();
    if (m_poissonRatio) m_poissonRatio->clear();
    if (m_thermalExpansionCoeff) m_thermalExpansionCoeff->clear();
    if (m_bendRadius) m_bendRadius->clear();

    // --- ДОБАВИТЬ ПРОВЕРКИ И ДЛЯ setValue ---
    if (m_pressure) m_pressure->setValue(10.0);
    if (m_massFlow) m_massFlow->setValue(50.0);
    if (m_operationalFactor) m_operationalFactor->setValue(1.0);
    if (m_reliabilityYield) m_reliabilityYield->setValue(1.0);
    if (m_reliabilityStrength) m_reliabilityStrength->setValue(1.0);
    if (m_responsibilityFactor) m_responsibilityFactor->setValue(1.0);
    if (m_pressureReliability) m_pressureReliability->setValue(1.0);

    if (m_density) m_density->setValue(850.0);
    if (m_yieldStrength) m_yieldStrength->setValue(600.0);
    if (m_tensileStrength) m_tensileStrength->setValue(600.0);
    if (m_fluidBulkModulus) m_fluidBulkModulus->setValue(3000.0);
    if (m_steelYoungModulus) m_steelYoungModulus->setValue(3000.0);
    if (m_temperatureDelta) m_temperatureDelta->setValue(0.0);
    if (m_poissonRatio) m_poissonRatio->setValue(2.0);
    if (m_thermalExpansionCoeff) m_thermalExpansionCoeff->setValue(0.0);
    if (m_bendRadius) m_bendRadius->setValue(0.0);
}
