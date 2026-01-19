#include "resultpage.h"
#include "pipelineparameters.h"
#include "interaction.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QDebug>
#include <QGraphicsRectItem>
#include <QResizeEvent>
#include <QElapsedTimer>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QShowEvent>
#include <QShortcut>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QMenu>

// Конструктор класса ResultPage
ResultPage::ResultPage(QWidget *parent)
    : QWidget(parent)
    , m_resultLabel(nullptr)
    , m_graphicsView(nullptr)
    , m_restartBtn(nullptr)
    , m_exitBtn(nullptr)
    , m_scene(nullptr)
    , m_interaction(nullptr)
    , m_animationTimer(nullptr)  // Инициализация указателя на таймер анимации
    , m_frameCounter(0)  // Инициализация счетчика кадров
{

    setupUI();

    m_interaction = new Interaction(m_scene, this);

    setFocusPolicy(Qt::StrongFocus);

    m_restartBtn->installEventFilter(this);
    m_exitBtn->installEventFilter(this);
}

// Деструктор класса ResultPage
ResultPage::~ResultPage()
{
    clearPage();  // Вызываем очистку, которая корректно завершит таймеры и удалит объекты
}

// Метод настройки пользовательского интерфейса
void ResultPage::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);  // Создание основного вертикального layout
    mainLayout->setContentsMargins(0, 0, 0, 0);  // Установка нулевых отступов по краям

    // Создаем меню
    m_menuBar = new QMenuBar(this);
    m_menuBar->setStyleSheet("QMenuBar { background-color: #f0f0f0; }");

    // Создаем меню "Сохранить"
    m_saveMenu = m_menuBar->addMenu("Сохранить");

    // Добавляем действия в меню "Сохранить"
    m_saveCalculationsAction = new QAction("Сохранить вычисления в .txt", this);
    m_saveImageAction = new QAction("Сохранить изображение в .png", this);

    m_saveMenu->addAction(m_saveCalculationsAction);
    m_saveMenu->addAction(m_saveImageAction);

    // Подключение сигналов
    connect(m_saveCalculationsAction, &QAction::triggered, this, &ResultPage::onSaveCalculationsClicked);
    connect(m_saveImageAction, &QAction::triggered, this, &ResultPage::onSaveImageClicked);

    mainLayout->setMenuBar(m_menuBar);
    mainLayout->addSpacing(5);  // Добавление отступа после меню

    //отображение текстового результата расчета
    m_resultLabel = new QLabel(this);
    m_resultLabel->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    m_resultLabel->setWordWrap(true);  // Разрешение переноса текста на новую строку
    m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;");
    mainLayout->addWidget(m_resultLabel);

    // GraphicsView для изображения трубопровода
    m_graphicsView = new QGraphicsView(this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);  // Включение сглаживания графики
    m_graphicsView->setStyleSheet("background: transparent; border: 1px solid #ccc;");
    mainLayout->addWidget(m_graphicsView);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Отключение горизонтальной полосы прокрутки
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // Отключение вертикальной полосы прокрутки
    m_graphicsView->setAlignment(Qt::AlignCenter);

    // Создаем графическую сцену
    m_scene = new QGraphicsScene(this);
    m_graphicsView->setScene(m_scene);

    // Кнопки управления
    m_restartBtn = new QPushButton("Начать заново");
    m_exitBtn = new QPushButton("Выход");

    connect(m_restartBtn, &QPushButton::clicked, this, &ResultPage::onRestartClicked);
    connect(m_exitBtn, &QPushButton::clicked, this, &ResultPage::onExitClicked);

    // Создание горизонтального layout
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_restartBtn);
    buttonLayout->addWidget(m_exitBtn);
    buttonLayout->setSpacing(20);  //расстояния между кнопками
    buttonLayout->setContentsMargins(20, 10, 20, 20);  // Установка отступов (лево, верх, право, низ)
    mainLayout->addLayout(buttonLayout);

    //"Стрелка влево" для выбора предыдущего сегмента
    auto* leftShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(leftShortcut, &QShortcut::activated, this, [this]() {
        qDebug() << "Left shortcut activated";
        if (m_interaction) m_interaction->selectPrevSegment();  // Вызов метода выбора предыдущего сегмента
    });

    //"Стрелка вправо" для выбора следующего сегмента
    auto* rightShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(rightShortcut, &QShortcut::activated, this, [this]() {
        qDebug() << "Right shortcut activated";
        if (m_interaction) m_interaction->selectNextSegment();  // Вызов метода выбора следующего сегмента
    });

    // Горячая клавиша "Стрелка вниз" для переключения отображения информации
    auto* downShortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    connect(downShortcut, &QShortcut::activated, this, [this]() {
        qDebug() << "Down shortcut activated";
        if (m_interaction) m_interaction->toggleInfo();  // Вызов метода переключения информации
    });
}

// Метод очистки страницы результатов
void ResultPage::clearPage()
{
    // 1. Останавливаем и удаляем таймер анимации
    if (m_animationTimer) {
        m_animationTimer->stop();
        delete m_animationTimer;
        m_animationTimer = nullptr;
    }

    // 2. Очищаем капли (убираем все элементы, связанные с таймером)
    cleanupOilDrops();

    // 3. Очищаем взаимодействие
    if (m_interaction) {
        m_interaction->clear();
    }

    // 4. Теперь можно безопасно очищать сцену
    if (m_scene) {
        m_scene->clear();  // Это удалит все элементы, включая bgItem
    }

    // 5. Очищаем остальные данные
    m_diameters.clear();
    m_pipeSegments.clear();
    m_validationResults.clear();

    if (m_resultLabel) {
        m_resultLabel->clear();
    }

    qDebug() << "ResultPage: page cleared";
}
// Метод очистки капель нефти из анимации
void ResultPage::cleanupOilDrops()
{
    // Удаляем все капли из сцены
    for (const SimpleDrop& drop : m_activeDrops) {
        if (drop.item && drop.item->scene()) {
            drop.item->scene()->removeItem(drop.item);  // Удаление капли из сцены
            delete drop.item;  // Освобождение памяти капли
        }
    }

    m_activeDrops.clear();
}

// Обработчик события изменения размера окна
void ResultPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Автоматическое масштабирование графической сцены при изменении размера окна
    if (m_scene && !m_scene->items().isEmpty()) {
        m_graphicsView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);  // Подгонка сцены под размер окна с сохранением пропорций
    }
}

// Слот обработки нажатия кнопки "Начать заново"
void ResultPage::onRestartClicked()
{
    clearPage();

    emit restartRequested();
}

// Слот обработки нажатия кнопки "Выход"
void ResultPage::onExitClicked()
{
    emit exitRequested();
}

// Слот обработки выбора пункта меню "Сохранить вычисления"
void ResultPage::onSaveCalculationsClicked()
{
    saveCalculationsToTxt();
}

// Слот обработки выбора пункта меню "Сохранить изображение"
void ResultPage::onSaveImageClicked()
{
    saveImageToPng();
}

// Метод сохранения результатов расчета в текстовый файл
void ResultPage::saveCalculationsToTxt()
{
    if (m_validationResults.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Нет данных для сохранения!");  // Показ сообщения об ошибке
        return;
    }

    // Запрашиваем у пользователя место сохранения файла
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Сохранить вычисления",
                                                    QDir::homePath() + "/результаты_расчета_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"),
                                                    "Текстовые файлы (*.txt)");

    if (fileName.isEmpty()) {
        return;
    }

    // Добавляем расширение .txt если его нет в имени файла
    if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
        fileName += ".txt";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // Попытка открыть файл для записи
        QMessageBox::critical(this, "Ошибка",
                              QString("Не удалось открыть файл для записи:\n%1").arg(file.errorString()));  // Показ сообщения об ошибке
        return;
    }

    QTextStream out(&file);  // Создание текстового потока для записи в файл
// Для Qt 6 используем setEncoding вместо setCodec
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif

    // Вспомогательная лямбда-функция для создания строк-разделителей
    auto createSeparator = [](int length, const QString& symbol) -> QString {
        QString result;  // Строка результата
        for (int i = 0; i < length; i++) {  // Цикл добавления символов
            result += symbol;
        }
        return result;
    };

    int lineWidth = 80;

    // Заголовок отчета
    out << createSeparator(lineWidth, "=") << "\n";  // Верхняя граница
    out << "РЕЗУЛЬТАТЫ РАСЧЕТА ОПТИМАЛЬНОГО ДИАМЕТРА НЕФТЕПРОВОДА\n";  // Заголовок
    out << createSeparator(lineWidth, "=") << "\n\n";  // Нижняя граница

    // Информация о пользователе и времени
    out << "Пользователь: " << m_userName << "\n";  // Имя пользователя
    out << "Дата и время расчета: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss") << "\n";  // Дата и время
    out << "Режим расчета: " << (m_mode == Mode::Mode1 ? "Режим 1 (типовые параметры)" : "Режим 2 (пользовательский ввод)") << "\n\n";  // Режим расчета

    // Раздел введенных параметров
    out << createSeparator(lineWidth, "-") << "\n";
    out << "ВВЕДЕННЫЕ ПАРАМЕТРЫ\n";
    out << createSeparator(lineWidth, "-") << "\n";

    // Вывод основных параметров расчета
    out << "Эксплуатационное давление: " << m_params.pressure << " МПа\n";
    out << "Массовый расход: " << m_params.massFlow << " кг/с\n";
    out << "Количество диаметров для анализа: " << m_params.outerDiameters.size() << "\n";
    out << "Наружные диаметры (мм): ";
    for (int i = 0; i < m_params.outerDiameters.size(); ++i) {
        out << m_params.outerDiameters[i];  // Вывод каждого диаметра
        if (i < m_params.outerDiameters.size() - 1) out << ", ";  // Добавление запятой между диаметрами
    }
    out << "\n\n";

    // Вывод коэффициентов надежности
    out << "Коэффициент условий работы (m): " << m_params.operationalFactor << "\n";
    out << "Коэффициент надёжности по текучести (y_my): " << m_params.reliabilityYield << "\n";
    out << "Коэффициент надёжности по прочности (y_mu): " << m_params.reliabilityStrength << "\n";
    out << "Коэффициент надёжности по ответственности (y_n): " << m_params.responsibilityFactor << "\n";
    out << "Коэффициент надёжности по давлению (y_fp): " << m_params.pressureReliability << "\n\n";

    // Вывод параметров в зависимости от режима расчета
    if (m_mode == Mode::Mode1) {
        out << "ТИПОВЫЕ ПАРАМЕТРЫ (РЕЖИМ 1):\n";
        out << "Плотность: " << m_params.density << " кг/м³\n";
        out << "Предел текучести: " << m_params.yieldStrength << " МПа\n";
        out << "Предел прочности: " << m_params.tensileStrength << " МПа\n";
        out << "Модуль упругости среды: " << m_params.fluidBulkModulus << " МПа\n";
        out << "Модуль упругости стали: " << m_params.steelYoungModulus << " МПа\n";
        out << "Температурный перепад: " << m_params.temperatureDelta << " °C\n";
        out << "Коэффициент Пуассона: " << m_params.poissonRatio << "\n";
        out << "Коэффициент линейного расширения: " << m_params.thermalExpansionCoeff << " 1/°C\n";
        out << "Радиус изгиба: " << m_params.bendRadius << " м\n\n";
    } else {
        out << "ПОЛЬЗОВАТЕЛЬСКИЕ ПАРАМЕТРЫ (РЕЖИМ 2):\n";
        // Те же параметры, но с пользовательскими значениями
        out << "Плотность: " << m_params.density << " кг/м³\n";
        out << "Предел текучести: " << m_params.yieldStrength << " МПа\n";
        out << "Предел прочности: " << m_params.tensileStrength << " МПа\n";
        out << "Модуль упругости среды: " << m_params.fluidBulkModulus << " МПа\n";
        out << "Модуль упругости стали: " << m_params.steelYoungModulus << " МПа\n";
        out << "Температурный перепад: " << m_params.temperatureDelta << " °C\n";
        out << "Коэффициент Пуассона: " << m_params.poissonRatio << "\n";
        out << "Коэффициент линейного расширения: " << m_params.thermalExpansionCoeff << " 1/°C\n";
        out << "Радиус изгиба: " << m_params.bendRadius << " м\n\n";
    }

    // Раздел результатов для каждого диаметра
    out << createSeparator(lineWidth, "-") << "\n";
    out << "РЕЗУЛЬТАТЫ ДЛЯ КАЖДОГО ДИАМЕТРА\n";
    out << createSeparator(lineWidth, "-") << "\n\n";

    // Цикл по всем результатам валидации для каждого диаметра
    for (const ValidationResult& res : m_validationResults) {
        out << "Диаметр: " << res.diameter << " мм\n";  // Вывод диаметра
        out << "Статус: ";  // Вывод статуса диаметра

        // Определение и вывод статуса диаметра
        if (res.isOptimal) {
            out << "ОПТИМАЛЬНЫЙ\n";
        } else if (res.satisfiesFlowSpeed && res.satisfiesHoopStress &&
                   res.satisfiesAxialStress && res.satisfiesEquivalentStress) {
            out << " ПОДХОДИТ\n";
        } else {
            out << "НЕ ПОДХОДИТ\n";
        }

        // Вывод скорости потока с проверкой соответствия диапазону
        out << "Скорость потока: " << res.flowSpeed << " м/с";
        if (res.satisfiesFlowSpeed) {
            out << " (в диапазоне 1.0-3.0 м/с)\n";
        } else {
            out << " (вне диапазона 1.0-3.0 м/с)\n";
        }

        // Вывод детальной информации для подходящих диаметров
        if (res.isOptimal || (res.satisfiesFlowSpeed && res.satisfiesHoopStress &&
                              res.satisfiesAxialStress && res.satisfiesEquivalentStress)) {
            out << "Толщина стенки: " << res.finalThickness * 1000 << " мм\n";  // Толщина стенки (перевод из метров в мм)
            out << "Коэффициент запаса (кольцевое напряжение): " << res.safetyHoop << "\n";
            out << "Коэффициент запаса (осевое напряжение): " << res.safetyAxial << "\n";
            out << "Коэффициент запаса (эквивалентное напряжение): " << res.safetyEquivalent << "\n";

            double minSafety = std::min({res.safetyHoop, res.safetyAxial, res.safetyEquivalent});  // Нахождение минимального коэффициента запаса
            out << "Минимальный коэффициент запаса: " << minSafety << "\n";
        } else {
            // Для неподходящих диаметров - информация не рассчитывалась
            out << "Толщина стенки: не рассчитана\n";
            out << "Коэффициенты запаса: не рассчитаны (диаметр не подходит по скорости потока)\n";
            out << "Минимальный коэффициент запаса: не рассчитан\n";
        }

        out << "\n" << createSeparator(60, "~") << "\n\n";  // Разделитель между разными диаметрами
    }

    // Раздел итогового результата
    out << createSeparator(lineWidth, "-") << "\n";
    out << "ИТОГОВЫЙ РЕЗУЛЬТАТ\n";
    out << createSeparator(lineWidth, "-") << "\n\n";

    bool foundOptimal = false;
    // Поиск оптимального диаметра среди всех результатов
    for (const ValidationResult& res : m_validationResults) {
        if (res.isOptimal) {
            out << "ОПТИМАЛЬНЫЙ ДИАМЕТР: " << res.diameter << " мм\n";
            out << "Толщина стенки: " << res.finalThickness * 1000 << " мм\n";
            out << "Коэффициент запаса (кольцевое напряжение): " << res.safetyHoop << "\n";
            out << "Коэффициент запаса (осевое напряжение): " << res.safetyAxial << "\n";
            out << "Коэффициент запаса (эквивалентное напряжение): " << res.safetyEquivalent << "\n";

            double minSafety = std::min({res.safetyHoop, res.safetyAxial, res.safetyEquivalent});
            out << "Минимальный коэффициент запаса: " << minSafety << "\n";
            foundOptimal = true;  // Установка флага, что оптимальный диаметр найден
            break;  // Выход из цикла после нахождения первого оптимального диаметра
        }
    }

    // Если оптимальный диаметр не найден
    if (!foundOptimal) {
        out << "ОПТИМАЛЬНЫЙ ДИАМЕТР: НЕ НАЙДЕН\n";
        out << "Ни один из предложенных диаметров не удовлетворяет всем условиям.\n";
    }

    // Заключительная часть отчета
    out << "\n" << createSeparator(lineWidth, "=") << "\n";
    out << "РАСЧЕТ ЗАВЕРШЕН\n";
    out << createSeparator(lineWidth, "=") << "\n";

    file.close();  // Закрытие файла

    // Показать сообщение об успешном сохранении
    QMessageBox::information(this, "Успех",
                             QString("Результаты успешно сохранены в файл:\n%1").arg(fileName));
}

// Метод сохранения графической схемы трубопровода в PNG файл
void ResultPage::saveImageToPng()
{
    // Проверка наличия сцены и элементов в ней
    if (!m_scene || m_scene->items().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Нет изображения для сохранения!");
        return;
    }

    // Запрашиваем у пользователя место сохранения файла с автосгенерированным именем
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Сохранить изображение",
                                                    QDir::homePath() + "/схема_трубопровода_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"),
                                                    "Изображения PNG (*.png)");

    if (fileName.isEmpty()) {  // Если пользователь отменил диалог
        return;
    }

    // Добавляем расширение .png если его нет в имени файла
    if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
        fileName += ".png";
    }

    // Получаем размер сцены для создания изображения соответствующего размера
    QRectF sceneRect = m_scene->sceneRect();
    if (sceneRect.isEmpty()) {  // Если сцена пустая, устанавливаем размер по умолчанию
        sceneRect = QRectF(0, 0, 800, 400);
    }

    // Создаем изображение с альфа-каналом (Format_ARGB32 поддерживает прозрачность)
    QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);  // Заполняем белым фоном

    // Создаем объект QPainter для рисования на изображении
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);  // Включаем сглаживание линий
    painter.setRenderHint(QPainter::TextAntialiasing);  // Включаем сглаживание текста

    // Рендерим всю сцену в изображение с масштабом 1:1
    m_scene->render(&painter, QRectF(), sceneRect);
    painter.end();  // Завершаем рисование

    // Сохраняем изображение в файл формата PNG
    if (image.save(fileName, "PNG")) {
        QMessageBox::information(this, "Успех",
                                 QString("Изображение успешно сохранено в файл:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить изображение!");
    }
}

// Основной метод установки результатов расчета на страницу
void ResultPage::setResults(const QString &optimalDiameter, const QString &optimalThickness,
                            const QString &safetyHoop, const QString &safetyAxial,
                            const QString &safetyEquivalent, const QString &minSafety,
                            const QVector<double>& diameters, const QPixmap &backgroundImage,
                            const QVector<ValidationResult>& validationResults)
{
    // Очищаем предыдущие результаты перед установкой новых
    clearPage();

    // СОХРАНЯЕМ РЕЗУЛЬТАТЫ РАСЧЕТА во внутренние переменные класса
    m_validationResults = validationResults;

    // Устанавливаем текст результата в главный лейбл
    QString resultText = QString("Оптимальный диаметр: %1 мм\n").arg(optimalDiameter);
    m_resultLabel->setText(resultText);

    // Создаем визуализацию трубопровода на основе диаметров и фонового изображения
    createPipelineVisualization(diameters, backgroundImage);

    m_diameters = diameters;  // Сохраняем диаметры для использования в анимации

    // Настраиваем интерактивность сцены
    if (m_interaction) {
        m_interaction->setup(m_pipeSegments, diameters, validationResults);
    }

    // Запускаем анимацию потока нефти, если есть хотя бы один диаметр
    if (!diameters.isEmpty()) {
        startOilAnimation();
    }

    setFocus();  // Устанавливаем фокус на виджет для обработки горячих клавиш
}

// Метод сохранения пользовательских данных (используется при сохранении отчета)
void ResultPage::setUserData(const QString& userName, Mode mode, const PipelineParameters& params)
{
    m_userName = userName;  // Сохраняем имя пользователя
    m_mode = mode;          // Сохраняем режим расчета (Mode1 или Mode2)
    m_params = params;      // Сохраняем все параметры расчета
}

// Метод создания визуализации трубопровода на графической сцене
void ResultPage::createPipelineVisualization(const QVector<double>& diameters, const QPixmap &backgroundImage)
{
    // Получаем текущие размеры области просмотра (View) для расчета масштабов
    int viewWidth = m_graphicsView->viewport()->width();
    int viewHeight = m_graphicsView->viewport()->height();

    if (viewWidth <= 0 || viewHeight <= 0) {  // Проверка корректности размеров
        return;
    }

    int baseHeight = 60;  // Базовая высота сегмента трубы (пиксели)
    double sum = 0;
    // Вычисляем сумму всех диаметров для расчета среднего значения
    for (double d : diameters) {
        sum += d;
    }
    // Вычисляем средний диаметр (если диаметры есть, иначе 0)
    double averageD = diameters.isEmpty() ? 0 : sum / diameters.size();

    // Определяем вертикальное положение оси трубопровода (3/4 от высоты окна)
    int verticalAxisY = 3 * viewHeight / 4;
    int connectorWidth = 10;    // Ширина соединительного элемента между трубами
    int connectorHeight = 70;   // Высота соединительного элемента

    // Рассчитываем ширину одного сегмента трубы
    // Формула: (общая ширина View - ширина всех соединителей) / количество сегментов
    int segmentWidth = diameters.isEmpty() ? 0 :
                           (viewWidth - (diameters.size() - 1) * connectorWidth) / diameters.size();

    if (segmentWidth < 10) {  // Минимальная ширина сегмента
        segmentWidth = 10;
    }

    // Рассчитываем общую ширину сцены с учетом всех сегментов и соединителей
    int sceneWidth = diameters.isEmpty() ? viewWidth :
                         (diameters.size() * segmentWidth) + ((diameters.size() - 1) * connectorWidth);

    // Масштабируем фоновое изображение под размеры сцены
    QPixmap scaledBackground = backgroundImage.scaled(sceneWidth, viewHeight,
                                                      Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_scene->addPixmap(scaledBackground)->setPos(0, 0);

    // Создаем информацию о сегментах труб
    m_pipeSegments.clear();  // Очищаем предыдущие данные

    int x = 0;  // Текущая координата X для отрисовки
    for (int i = 0; i < diameters.size(); ++i) {  // Цикл по всем диаметрам
        double diameter = diameters[i];
        // Вычисляем соотношение текущего диаметра к среднему
        double ratio = diameter / (averageD > 0 ? averageD : 1);
        // Высота сегмента пропорциональна диаметру трубы
        int segmentHeight = static_cast<int>(ratio * baseHeight);

        if(segmentHeight < 18){  // Минимальная высота сегмента
            segmentHeight = 18;
        }

        // Вычисляем Y-координату для центрирования сегмента относительно вертикальной оси
        int segmentY = verticalAxisY - segmentHeight / 2;

        // Внешний прямоугольник трубы (корпус)
        QRectF segmentRect(x, segmentY, segmentWidth, segmentHeight);
        QGraphicsRectItem *segment = m_scene->addRect(segmentRect, QPen(Qt::NoPen), Qt::darkGray);

        // Внутренний прямоугольник трубы (полость)
        double marginRatio = 0.15;  // Отступ от краев (15%)
        int innerMargin = static_cast<int>(segmentHeight * marginRatio);

        // Ограничения на отступы
        if (innerMargin < 4) innerMargin = 4;
        if (innerMargin > 20) innerMargin = 20;

        // Создаем структуру информации о сегменте
        PipeSegmentInfo segmentInfo;
        segmentInfo.segmentIndex = i;  // Индекс сегмента
        segmentInfo.leftX = x;         // Левая граница сегмента
        segmentInfo.rightX = x + segmentWidth;  // Правая граница сегмента

        // Если высота сегмента позволяет создать внутреннюю полость
        if (segmentHeight > innerMargin * 2) {
            QRectF innerRect(x + 2,  // Отступ от левого края
                             segmentY + innerMargin + 2,  // Отступ от верхнего края
                             segmentWidth - 4,  // Ширина с учетом отступов
                             segmentHeight - 2 * innerMargin - 4);  // Высота с учетом отступов

            QGraphicsRectItem *innerSegment = m_scene->addRect(innerRect,
                                                               QPen(Qt::NoPen),
                                                               QBrush(QColor(100, 100, 100)));
            segmentInfo.innerRect = innerRect;  // Сохраняем внутренний прямоугольник
        } else {
            // Если сегмент слишком тонкий, внутренний прямоугольник равен внешнему с небольшим отступом
            segmentInfo.innerRect = segmentRect.adjusted(2, 2, -2, -2);
        }

        m_pipeSegments.append(segmentInfo);  // Добавляем сегмент в список

        // Добавляем соединительный элемент между трубами (кроме последней трубы)
        if (i < diameters.size() - 1) {
            int connectorY = verticalAxisY - connectorHeight / 2;  // Центрируем соединитель
            QRectF connectorRect(x + segmentWidth, connectorY, connectorWidth, connectorHeight);
            QGraphicsRectItem *connector = m_scene->addRect(connectorRect, QPen(Qt::NoPen), Qt::lightGray);
            connector->setZValue(100);  // Высокий z-value чтобы был поверх других элементов
            x += segmentWidth + connectorWidth;  // Сдвигаем X для следующего элемента
        } else {
            x += segmentWidth;  // Последняя труба, сдвигаем только на ширину трубы
        }
    }

    // Устанавливаем размер сцены равным рассчитанным размерам
    m_scene->setSceneRect(0, 0, sceneWidth, viewHeight);

    // Масштабируем сцену под размер View с сохранением пропорций
    m_graphicsView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_graphicsView->centerOn(m_scene->sceneRect().center());  // Центрируем сцену
}

// Метод запуска анимации потока нефти
void ResultPage::startOilAnimation()
{
    // ВСЕГДА останавливаем и удаляем старый таймер (если был)
    if (m_animationTimer) {
        m_animationTimer->stop();
        delete m_animationTimer;
        m_animationTimer = nullptr;
    }

    // Очищаем старые капли
    cleanupOilDrops();

    // Сбрасываем счетчик
    m_frameCounter = 0;

    // Создаем новый таймер
    m_animationTimer = new QTimer(this);

    // Подключаем таймер к анонимной лямбде
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        if (!m_scene || !m_animationTimer || !m_animationTimer->isActive()) {
            return; // Защита от вызова, если сцена или таймер уже уничтожены
        }
        updateOilAnimation();

        m_frameCounter++;
        if (m_frameCounter >= 20) {
            createOilDrop();
            m_frameCounter = 0;
        }
    });

    m_animationTimer->start(50); // 60 FPS
}
// Метод обновления анимации капель нефти
void ResultPage::updateOilAnimation()
{
    // --- ПРОВЕРКА НА ВАЛИДНОСТЬ СЦЕНЫ И ТАЙМЕРА ---
    if (!m_scene || !m_animationTimer || !m_animationTimer->isActive()) {
        qDebug() << "updateOilAnimation: scene or timer invalid, stopping.";
        if (m_animationTimer) {
            m_animationTimer->stop();
        }
        return;
    }

    // --- ПРОВЕРКА НА ПУСТУЮ СЦЕНУ ---
    if (m_scene->items().isEmpty()) {
        qDebug() << "updateOilAnimation: scene is empty, stopping timer.";
        m_animationTimer->stop();
        return;
    }

    // --- ПРОВЕРКА НА ОТСУТСТВИЕ СЕГМЕНТОВ ---
    if (m_pipeSegments.isEmpty()) {
        qDebug() << "updateOilAnimation: no pipe segments, stopping timer.";
        m_animationTimer->stop();
        return;
    }

    // --- ЛОГИРОВАНИЕ ШИРИНЫ СЦЕНЫ ---
    qDebug() << "updateOilAnimation: scene width =" << m_scene->width();

    // --- ОБХОД В ОБРАТНОМ ПОРЯДКЕ ДЛЯ БЕЗОПАСНОГО УДАЛЕНИЯ ---
    for (int i = m_activeDrops.size() - 1; i >= 0; --i) {
        SimpleDrop& drop = m_activeDrops[i];

        // --- ПРОВЕРКА НА СУЩЕСТВОВАНИЕ ЭЛЕМЕНТА ---
        if (!drop.item || !drop.item->scene()) {
            m_activeDrops.removeAt(i);
            continue;
        }

        // --- ОБНОВЛЕНИЕ ПОЗИЦИИ ---
        QPointF pos = drop.item->pos();
        qreal size = drop.item->rect().width();
        qreal centerX = pos.x() + size / 2;

        drop.item->moveBy((drop.speed / 30.0) * 2, 0);
        centerX += drop.speed / 30.0;

        // --- ЛОГИРОВАНИЕ ПОЗИЦИИ КАПЛИ ---
        qDebug() << "updateOilAnimation: drop" << i << "centerX =" << centerX << ", scene width =" << m_scene->width();

        // --- ПОИСК СЕГМЕНТА ---
        int closestSegment = -1;
        qreal minDistance = 1000000;

        for (int segIndex = 0; segIndex < m_pipeSegments.size(); ++segIndex) {
            const PipeSegmentInfo& segment = m_pipeSegments[segIndex];
            qreal segmentCenterX = (segment.leftX + segment.rightX) / 2;
            qreal distance = qAbs(centerX - segmentCenterX);

            if (distance < minDistance &&
                centerX >= segment.leftX - 20 &&
                centerX <= segment.rightX + 20) {
                minDistance = distance;
                closestSegment = segIndex;
            }
        }

        if (closestSegment != -1 && closestSegment != drop.currentSegment) {
            drop.currentSegment = closestSegment;
            const PipeSegmentInfo& segment = m_pipeSegments[closestSegment];

            qreal pipeHeight = segment.innerRect.height();
            qreal newSize = pipeHeight * 0.8;
            if (newSize < 6) newSize = 6;

            qreal oldSize = size;

            if (qAbs(newSize - oldSize) > 1.0) {
                qreal targetY = segment.innerRect.center().y() - newSize / 2;
                qreal newX = drop.item->x();
                drop.item->setRect(0, 0, newSize, newSize);
                drop.item->setPos(newX, targetY);
                drop.speed = 40.0 + (pipeHeight * 0.8);
            }
        }

        // --- УДАЛЕНИЕ КАПЛИ, ЕСЛИ ВЫШЛА ЗА ПРЕДЕЛЫ ---
        if (centerX > m_scene->width() || centerX < 0) {
            qDebug() << "updateOilAnimation: removing drop" << i << "at centerX =" << centerX;
            m_scene->removeItem(drop.item);
            delete drop.item;
            m_activeDrops.removeAt(i);
        }
    }
}
// Метод создания новой капли нефти
void ResultPage::createOilDrop()
{
    qDebug() << "createOilDrop called";

    // --- ПРОВЕРКА НА ВАЛИДНОСТЬ ---
    if (!m_scene || m_pipeSegments.isEmpty() || !m_animationTimer || !m_animationTimer->isActive()) {
        qDebug() << "createOilDrop: conditions not met, returning.";
        return;
    }

    // --- ОГРАНИЧЕНИЕ НА КОЛИЧЕСТВО КАПЕЛЬ ---
    if (m_activeDrops.size() > 100) {
        qDebug() << "createOilDrop: max drops reached, skipping.";
        return;
    }

    const PipeSegmentInfo& firstSegment = m_pipeSegments.first();
    qreal pipeHeight = firstSegment.innerRect.height();
    if (pipeHeight <= 0) {
        pipeHeight = 20;
    }

    qreal dropSize = pipeHeight * 0.8;
    if (dropSize < 6) dropSize = 6;

    qreal startX = firstSegment.leftX;
    qreal startY = firstSegment.innerRect.center().y() - dropSize / 2;

    QGraphicsEllipseItem* dropItem = new QGraphicsEllipseItem(0, 0, dropSize, dropSize);
    dropItem->setPos(startX, startY);

    dropItem->setBrush(QBrush(QColor(30, 25, 20)));
    dropItem->setPen(QPen(QColor(60, 60, 60), 1.5));
    dropItem->setZValue(50);

    m_scene->addItem(dropItem);

    qreal speed = 40.0 + (pipeHeight * 0.8);

    SimpleDrop drop;
    drop.item = dropItem;
    drop.speed = speed;
    drop.currentSegment = -1;

    m_activeDrops.append(drop);

    qDebug() << "createOilDrop: created new drop, total drops:" << m_activeDrops.size();
}
