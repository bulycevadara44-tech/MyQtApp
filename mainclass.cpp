#include "mainclass.h"
#include "loginpage.h"
#include "modeselectionpage.h"
#include "inputparameterspage.h"
#include "resultpage.h"
#include "pipelineoptimizer.h"
#include <QStackedWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QThread>
#include <QScreen>

// Конструктор главного класса приложения
MainClass::MainClass(QWidget *parent)
    : QMainWindow(parent)
{
    // Настройка цветовой палитры окна (светло-серый фон)
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(245, 245, 245));
    palette.setColor(QPalette::WindowText, Qt::black);
    this->setPalette(palette);

    setWindowTitle("Оптимизация нефтепровода");

    // Создаем stacked widget для переключения между страницами (мастер-форма)
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Инициализация всех страниц приложения
    m_loginPage = new LoginPage(this);           // Страница входа/логина
    m_modePage = new ModeSelectionPage(this);    // Страница выбора режима расчета
    m_inputPage = new InputParametersPage(this); // Страница ввода параметров
    m_resultPage = new ResultPage(this);         // Страница результатов

    // Добавление страниц в stacked widget в порядке их использования
    m_stackedWidget->addWidget(m_loginPage);
    m_stackedWidget->addWidget(m_modePage);
    m_stackedWidget->addWidget(m_inputPage);
    m_stackedWidget->addWidget(m_resultPage);

    // Установка начального размера окна (для страницы логина)
    setFixedSize(350, 200);
    centerWindow();  // Центрирование окна на экране

    // Подключение сигналов между страницами и слотами главного окна

    // Сигналы от страницы логина
    connect(m_loginPage, &LoginPage::nextRequested, this, &MainClass::onLoginNext);
    connect(m_loginPage, &LoginPage::cancelRequested, this, &MainClass::onLoginCancel);

    // Сигналы от страницы выбора режима
    connect(m_modePage, &ModeSelectionPage::nextRequested, this, &MainClass::onModeNext);
    connect(m_modePage, &ModeSelectionPage::backRequested, this, &MainClass::onModeBack);

    // Сигналы от страницы ввода параметров
    connect(m_inputPage, &InputParametersPage::nextRequested, this, &MainClass::onInputNext);
    connect(m_inputPage, &InputParametersPage::backRequested, this, &MainClass::onInputBack);

    // Сигналы от страницы результатов
    connect(m_resultPage, &ResultPage::restartRequested, this, &MainClass::onResultRestart);
    connect(m_resultPage, &ResultPage::exitRequested, this, &MainClass::onResultExit);
}

// Метод центрирования окна на экране
void MainClass::centerWindow()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());
}

// Деструктор главного класса
MainClass::~MainClass()
{
    qDebug() << "MainClass destructor";
    qDebug() << "MainClass destroyed";
}

// СЛОТ: обработка перехода с страницы логина
void MainClass::onLoginNext()
{
    m_userName = m_loginPage->userName();  // Сохраняем имя пользователя
    m_stackedWidget->setCurrentIndex(1);   // Переход к выбору режима
    setFixedSize(800, 600);               // Изменение размера окна для следующей страницы
    centerWindow();                        // Перецентрирование окна
}

// СЛОТ: обработка отмены на странице логина (выход из приложения)
void MainClass::onLoginCancel()
{
    close();  // Закрытие приложения
}

// СЛОТ: обработка перехода с страницы выбора режима
void MainClass::onModeNext()
{
    m_selectedMode = m_modePage->selectedMode();  // Сохраняем выбранный режим
    m_inputPage->setMode(m_selectedMode);         // Передаем режим на страницу ввода
    m_stackedWidget->setCurrentIndex(2);          // Переход к вводу параметров
    setFixedSize(700, 600);                      // Изменение размера окна
    centerWindow();
}

// СЛОТ: обработка возврата с страницы выбора режима
void MainClass::onModeBack()
{
    m_stackedWidget->setCurrentIndex(0);  // Возврат к логину
    setFixedSize(350, 200);              // Восстановление размера для страницы логина
    centerWindow();
}

// СЛОТ: обработка перехода с страницы ввода параметров (самый важный метод)
void MainClass::onInputNext()
{
    try {
        // Получаем параметры из формы ввода
        PipelineParameters params = m_inputPage->toParameters();

        // Сохраняем параметры для последующей передачи на страницу результатов
        m_currentParams = params;  // Сохраняем все параметры расчета

        // Переход на страницу результатов
        m_stackedWidget->setCurrentIndex(3);

        // Настройка размеров окна для страницы результатов
        setMinimumSize(300, 300);                      // Минимальный размер
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);  // Максимальный (не ограничен)
        resize(900, 700);                             // Начальный размер
        centerWindow();

        // Отладочное сообщение о начале расчета
        qDebug() << "MainClass: starting calculation...";

        // СОЗДАНИЕ ОБЪЕКТА ОПТИМИЗАТОРА И РАСЧЕТ РЕЗУЛЬТАТОВ
        PipelineOptimizer optimizer;
        auto results = optimizer.calculate(params);  // Основной расчет! Получаем validationResults

        // ПЕРЕМЕННЫЕ ДЛЯ ОТОБРАЖЕНИЯ РЕЗУЛЬТАТОВ
        QString optimalDiameter = "Не найден";
        QString optimalThickness = "N/A";
        QString safetyHoop = "N/A";
        QString safetyAxial = "N/A";
        QString safetyEquivalent = "N/A";
        QString minSafety = "N/A";

        // Берем диаметры из параметров для визуализации
        QVector<double> diametersForVisualization = params.outerDiameters;

        qDebug() << "=== ДИАМЕТРЫ ДЛЯ ВИЗУАЛИЗАЦИИ ===";
        qDebug() << "Из params.outerDiameters:" << diametersForVisualization;
        qDebug() << "Количество:" << diametersForVisualization.size();

        // ПОИСК ОПТИМАЛЬНОГО ДИАМЕТРА СРЕДИ РЕЗУЛЬТАТОВ РАСЧЕТА
        for (const auto& res : results) {
            if (res.isOptimal) {
                optimalDiameter = QString::number(res.diameter);
                optimalThickness = QString::number(res.finalThickness * 1000, 'f', 4);
                safetyHoop = QString::number(res.safetyHoop, 'f', 4);
                safetyAxial = QString::number(res.safetyAxial, 'f', 4);
                safetyEquivalent = QString::number(res.safetyEquivalent, 'f', 4);
                minSafety = QString::number(std::min({res.safetyHoop, res.safetyAxial, res.safetyEquivalent}), 'f', 4);
                break;  // Нашли оптимальный - выходим из цикла
            }
        }

        // ПОДГОТОВКА ФОНОВОГО ИЗОБРАЖЕНИЯ ДЛЯ ВИЗУАЛИЗАЦИИ
        QPixmap backgroundImage(":/image/gazoprovod.jpg");
        if (backgroundImage.isNull()) {
            // Если изображение не загружено, создаем стандартный фон
            qDebug() << "Background image not found, creating default";
            backgroundImage = QPixmap(800, 400);
            backgroundImage.fill(QColor(200, 220, 255));
            QPainter painter(&backgroundImage);
            painter.setPen(Qt::darkBlue);
            painter.setFont(QFont("Arial", 16, QFont::Bold));
            painter.drawText(backgroundImage.rect(), Qt::AlignCenter, "Схема трубопровода");
            painter.end();
        }

        qDebug() << "MainClass: calling setResults with" << diametersForVisualization.size() << "diameters";

        // ПЕРЕДАЧА РЕЗУЛЬТАТОВ НА СТРАНИЦУ РЕЗУЛЬТАТОВ
        if (m_resultPage && !diametersForVisualization.isEmpty()) {
            // 1. Передаем пользовательские данные (имя, режим, параметры)
            m_resultPage->setUserData(m_userName, m_selectedMode, m_currentParams);

            // 2. Передаем все рассчитанные результаты для отображения
            m_resultPage->setResults(optimalDiameter, optimalThickness, safetyHoop, safetyAxial,
                                     safetyEquivalent, minSafety, diametersForVisualization,
                                     backgroundImage, results);  // Ключевое: передаем ВСЕ результаты!
        }

        qDebug() << "MainClass: results set successfully";

    } catch (const std::exception& e) {
        // Обработка исключений при получении параметров
        QMessageBox::warning(this, "Ошибка", QString::fromStdString(e.what()));
    }
}

// СЛОТ: обработка возврата со страницы ввода параметров
void MainClass::onInputBack()
{
    m_stackedWidget->setCurrentIndex(1);  // Возврат к выбору режима
    setFixedSize(800, 600);              // Изменение размера окна
    centerWindow();

    // Очистка полей ввода на странице параметров
    if (m_inputPage) {
        m_inputPage->clearFields();
    }
}

// СЛОТ: обработка запроса на перезапуск с страницы результатов
void MainClass::onResultRestart()
{
    // Комплексная очистка всех страниц перед перезапуском

    // 1. Очистка страницы результатов
    if (m_resultPage) {
        m_resultPage->clearPage();
    }

    // 2. Очистка страницы ввода параметров
    if (m_inputPage) {
        m_inputPage->clearFields();
    }

    // 3. Очистка выбора режима
    if (m_modePage) {
        m_modePage->clearSelection();
    }
    if (m_loginPage) m_loginPage->clearFields();

    // 4. Возврат к началу (страница логина)
    m_stackedWidget->setCurrentIndex(0);
    setFixedSize(350, 200);  // Восстановление размера для страницы логина
    centerWindow();

    // 5. Очистка поля логина
    if (m_loginPage) {
        m_loginPage->clearFields();
    }
}

// СЛОТ: обработка запроса на выход с страницы результатов
void MainClass::onResultExit()
{
    // Очистка страницы результатов перед выходом
    if (m_resultPage) {
        m_resultPage->clearPage();
    }

    // Завершение работы приложения
    QApplication::quit();
}
