#include "modeselectionpage.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ModeSelectionPage::ModeSelectionPage(QWidget *parent)
    : QWidget(parent)
    , m_selectedMode(Mode::Mode1) // Изначально выбран первый режим
{
    // Создаем вертикальный layout для размещения элементов
    auto layout = new QVBoxLayout(this);

    // Создаем информационный текст с описанием режимов
    // Используется HTML-разметка для форматирования
    auto text_1 = new QLabel(
        "<b>Режим 1: Универсальный расчёт для типовых условий</b><br><br>"
        "Идеально подходит для предварительных расчётов и теоретической оценки оптимального диаметра трубопровода.<br>"
        "В этом режиме используются стандартные параметры, характерные для большинства нефтепроводов:<br><br>"
        "• <i>Нефтепродукт:</i> плотность ρ = 850 кг/м³, модуль упругости E₀ = 1300 МПа<br>"
        "• <i>Климатические условия:</i> умеренный климат с Δt = 20°C<br>"
        "• <i>Трасса:</i> прямолинейная (радиус изгиба r = 0 м)<br>"
        "• <i>Материал труб:</i> сталь 09Г2С<br>"
        "&nbsp;&nbsp;&nbsp;─ Предел текучести: σₜ = 343 МПа<br>"
        "&nbsp;&nbsp;&nbsp;─ Предел прочности: σₚ = 490 МПа<br>"
        "&nbsp;&nbsp;&nbsp;─ Модуль Юнга: E = 200×10³ МПа<br>"
        "&nbsp;&nbsp;&nbsp;─ Коэффициент Пуассона: μ = 0,3<br>"
        "&nbsp;&nbsp;&nbsp;─ Коэффициент теплового расширения: α = 11,4×10⁻⁶ 1/°C<br><br>"
        "<b>Режим 2: Расчёт для индивидуальных условий</b><br><br>"
        "Предназначен для точного инженерного расчёта с учётом специфических условий эксплуатации.<br>"
        "В этом режиме вы можете задать все параметры вручную:<br><br>"
        );

    // Создаем радио-кнопки для выбора режима
    m_mode1Radio = new QRadioButton("Режим 1", this);
    m_mode2Radio = new QRadioButton("Режим 2", this);
    m_mode1Radio->setChecked(true); // По умолчанию выбран режим 1

    // Добавляем элементы в layout
    layout->addWidget(text_1);
    layout->addWidget(new QLabel("Выберите режим расчёта:"));
    layout->addWidget(m_mode1Radio);
    layout->addWidget(m_mode2Radio);

    // Создаем кнопки навигации
    auto nextBtn = new QPushButton("Далее");
    auto backBtn = new QPushButton("Назад");
    layout->addWidget(nextBtn);
    layout->addWidget(backBtn);

    // Подключаем сигналы от радио-кнопок для обновления выбранного режима
    connect(m_mode1Radio, &QRadioButton::toggled, this, [this](bool c) {
        if (c) m_selectedMode = Mode::Mode1; // Если кнопка выбрана, устанавливаем режим 1
    });
    connect(m_mode2Radio, &QRadioButton::toggled, this, [this](bool c) {
        if (c) m_selectedMode = Mode::Mode2; // Если кнопка выбрана, устанавливаем режим 2
    });

    // Подключаем кнопки навигации
    connect(nextBtn, &QPushButton::clicked, this, &ModeSelectionPage::onNextClicked);
    connect(backBtn, &QPushButton::clicked, this, &ModeSelectionPage::onBackClicked);
}

// Возвращает текущий выбранный режим
Mode ModeSelectionPage::selectedMode() const
{
    return m_selectedMode;
}

// Обработчик кнопки "Далее"
void ModeSelectionPage::onNextClicked()
{
    emit nextRequested(); // Отправляем сигнал о переходе к следующей странице
}

// Обработчик кнопки "Назад"
void ModeSelectionPage::onBackClicked()
{
    emit backRequested(); // Отправляем сигнал о возврате к предыдущей странице
}

// Сброс выбора к значениям по умолчанию
void ModeSelectionPage::clearSelection()
{
    m_mode1Radio->setChecked(true); // Выбираем режим 1
    m_selectedMode = Mode::Mode1;   // Устанавливаем соответствующий режим
}
