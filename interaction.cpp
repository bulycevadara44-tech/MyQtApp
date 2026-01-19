#include "interaction.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QFont>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>


// Интерактивный прямоугольник для обработки событий мыши
class InteractiveRectItem : public QGraphicsRectItem {
public:
    InteractiveRectItem(const QRectF& rect, Interaction* parent, int index)
        : QGraphicsRectItem(rect), m_parent(parent), m_index(index)
    {
        setPen(Qt::NoPen);
        setBrush(Qt::transparent);
        setAcceptHoverEvents(true);
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
        Q_UNUSED(event);
        if (m_parent) {
            m_parent->onSegmentHoverEnter(m_index);
        }
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
        Q_UNUSED(event);
        if (m_parent) {
            m_parent->onSegmentHoverLeave(m_index);
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_parent) {
            m_parent->onSegmentClicked(m_index);
            event->accept();
        }
    }

private:
    Interaction* m_parent;
    int m_index;
};

Interaction::Interaction(QGraphicsScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_highlight(nullptr)
    , m_infoText(nullptr)
    , m_textBackground(nullptr)
    , m_hoveredIndex(-1)
    , m_selectedIndex(-1)
{
}
Interaction::~Interaction()
{
    clear(); // Убедитесь, что есть деструктор
}


void Interaction::setup(const QVector<PipeSegmentInfo>& pipeSegments,
                        const QVector<double>& diameters,
                        const QVector<ValidationResult>& validationResults)
{
    clear();

    m_pipeSegments = pipeSegments;
    m_diameters = diameters;
    m_validationResults = validationResults;

    // Создаем интерактивные области для каждого сегмента
    for (int i = 0; i < m_pipeSegments.size(); i++) {
        const PipeSegmentInfo& segment = m_pipeSegments[i];

        InteractiveRectItem* hitArea = new InteractiveRectItem(
            segment.innerRect, this, i
            );
        hitArea->setZValue(15);
        m_scene->addItem(hitArea);
        m_hitAreas.append(hitArea);
    }
}

void Interaction::clear()
{
    // Удаляем все интерактивные области
    for (QGraphicsRectItem* hitArea : m_hitAreas) {
        if (hitArea && hitArea->scene()) {
            hitArea->scene()->removeItem(hitArea);
        }
        delete hitArea;
    }
    m_hitAreas.clear();

    // УДАЛЯЕМ окно информации
    if (m_infoText) {
        if (m_infoText->scene()) {
            m_infoText->scene()->removeItem(m_infoText);
        }
        delete m_infoText;
        m_infoText = nullptr;
    }

    if (m_textBackground) {
        if (m_textBackground->scene()) {
            m_textBackground->scene()->removeItem(m_textBackground);
        }
        delete m_textBackground;
        m_textBackground = nullptr;
    }

    // УДАЛЯЕМ подсветку
    if (m_highlight) {
        if (m_scene && m_highlight->scene()) {  // <-- добавить проверку m_scene
            m_scene->removeItem(m_highlight);
        }
        delete m_highlight;
        m_highlight = nullptr;
    }

    m_pipeSegments.clear();
    m_diameters.clear();
    m_validationResults.clear();
    m_hoveredIndex = -1;
    m_selectedIndex = -1;
}
void Interaction::onSegmentHoverEnter(int index)
{
    if (index < 0 || index >= m_pipeSegments.size()) return;

    m_hoveredIndex = index;
    updateHighlight(index, true);
}

void Interaction::onSegmentHoverLeave(int index)
{
    if (index != m_hoveredIndex) return;

    m_hoveredIndex = -1;

    // Если это не выбранный сегмент - убираем подсветку
    if (index != m_selectedIndex) {
        updateHighlight(index, false);
    }

    hideSegmentInfo();
    m_selectedIndex = -1;
}

void Interaction::onSegmentClicked(int index)
{
    if (index < 0 || index >= m_pipeSegments.size()) return;

    // Если кликаем на тот же сегмент - скрываем/показываем
    if (index == m_selectedIndex) {
        // УДАЛЯЕМ текущее окно
        hideSegmentInfo();
        m_selectedIndex = -1;
        return;
    }

    // УДАЛЯЕМ старое окно перед созданием нового
    hideSegmentInfo();

    m_selectedIndex = index;
    showSegmentInfo(index);

    // Обновляем подсветку для выбранного сегмента
    updateHighlight(index, true);
}

QColor Interaction::getSegmentColor(int index) const
{
    if (index < 0 || index >= m_diameters.size()) {
        return Qt::gray;
    }

    double diameter = m_diameters[index];

    // Ищем результат для этого диаметра
    for (const ValidationResult& result : m_validationResults) {
        if (qFuzzyCompare(result.diameter, diameter)) {
            if (result.isOptimal) {
                return QColor(0, 255, 0);        // Зеленый - оптимальный
            } else if (result.satisfiesFlowSpeed &&
                       result.satisfiesHoopStress &&
                       result.satisfiesAxialStress &&
                       result.satisfiesEquivalentStress) {
                return QColor(255, 255, 0);     // Желтый - подходит
            } else {
                return QColor(255, 0, 0);       // Красный - не подходит
            }

        }
    }
}

void Interaction::updateHighlight(int index, bool show)
{
    if (index < 0 || index >= m_pipeSegments.size()) return;

    const PipeSegmentInfo& segment = m_pipeSegments[index];

    // Удаляем старую подсветку
    if (m_highlight) {
        m_scene->removeItem(m_highlight);
        delete m_highlight;
        m_highlight = nullptr;
    }

    // Создаем новую подсветку если нужно
    if (show) {
        QColor color = getSegmentColor(index);
        color.setAlpha(80); // Полупрозрачный

        m_highlight = m_scene->addRect(segment.innerRect,
                                       QPen(Qt::NoPen),
                                       color);
        m_highlight->setZValue(114);
    }
}

void Interaction::showSegmentInfo(int index)
{
    if (!m_scene) return;

    if (index < 0 || index >= m_diameters.size()) return;

    // Сначала скрываем предыдущую информацию
    hideSegmentInfo();

    double diameter = m_diameters[index];
    const PipeSegmentInfo& segment = m_pipeSegments[index];

    // Ищем результат для этого диаметра
    const ValidationResult* result = nullptr;
    for (const ValidationResult& res : m_validationResults) {
        if (qFuzzyCompare(res.diameter, diameter)) {
            result = &res;
            break;
        }
    }

    if (!result) return;

    // Формируем текст информации
    QString info;
    QString title;
    QColor textColor = Qt::white;

    if (result->isOptimal) {
        title = "ОПТИМАЛЬНЫЙ ДИАМЕТР";
        info = QString("Диаметр: %1 мм\n"
                       "Толщина стенки: %2 мм\n"
                       "Коэффициент запаса (кольцевое): %3\n"
                       "Коэффициент запаса (осевое): %4\n"
                       "Коэффициент запаса (эквивалентное): %5\n"
                       "Минимальный коэффициент: %6")
                   .arg(diameter)
                   .arg(result->finalThickness * 1000, 0, 'f', 2)
                   .arg(result->safetyHoop, 0, 'f', 2)
                   .arg(result->safetyAxial, 0, 'f', 2)
                   .arg(result->safetyEquivalent, 0, 'f', 2)
                   .arg(qMin(qMin(result->safetyHoop, result->safetyAxial),
                             result->safetyEquivalent), 0, 'f', 2);
        textColor = QColor(0, 200, 0); // Зеленый текст для оптимального
    } else if (result->satisfiesFlowSpeed &&
               result->satisfiesHoopStress &&
               result->satisfiesAxialStress &&
               result->satisfiesEquivalentStress) {
        title = "ПОДХОДИТ";
        double minSafety = qMin(qMin(result->safetyHoop, result->safetyAxial), result->safetyEquivalent);
        info = QString("Диаметр: %1 мм\n"
                       "Толщина стенки: %2 мм\n"
                       "Минимальный коэффициент запаса: %3")
                   .arg(diameter)
                   .arg(result->finalThickness * 1000, 0, 'f', 2)
                   .arg(minSafety, 0, 'f', 2);
        textColor = QColor(255, 255, 0);
    } else {
        title = "НЕ ПОДХОДИТ";
        QString reason = "Скорость потока не в диапазоне 1.0-3.0 м/с";

        info = QString("Диаметр: %1 мм\n"
                       "Скорость потока: %2 м/с\n"
                       "Не в диапазоне 1.0-3.0 м/с")
                   .arg(diameter)
                   .arg(result->flowSpeed, 0, 'f', 2);
        textColor = QColor(255, 100, 100); // Красный текст
    }

    // Создаем текст
    m_infoText = m_scene->addText(title + "\n" + info);
    m_infoText->setDefaultTextColor(textColor);
    m_infoText->setFont(QFont("Arial", 10));

    // Создаем фон для текста
    QRectF textRect = m_infoText->boundingRect();
    m_textBackground = m_scene->addRect(textRect,
                                        QPen(Qt::black, 1),
                                        QColor(30, 30, 30, 230));
    m_textBackground->setZValue(120);
    m_infoText->setZValue(121);

    // Позиционируем окно рядом с сегментом
    qreal textX = segment.rightX + 10;
    qreal textY = segment.innerRect.center().y() - textRect.height() / 2;

    // Если не помещается справа - показываем слева
    if (textX + textRect.width() > m_scene->width() - 10) {
        textX = segment.leftX - textRect.width() - 10;
    }

    // Корректируем по вертикали
    if (textY < 10) textY = 10;
    if (textY + textRect.height() > m_scene->height() - 10) {
        textY = m_scene->height() - textRect.height() - 10;
    }

    m_infoText->setPos(textX, textY);
    m_textBackground->setPos(textX, textY);
}

void Interaction::hideSegmentInfo()
{
    if (m_infoText) {
        m_scene->removeItem(m_infoText);
        delete m_infoText;
        m_infoText = nullptr;
    }

    if (m_textBackground) {
        m_scene->removeItem(m_textBackground);
        delete m_textBackground;
        m_textBackground = nullptr;
    }
}

void Interaction::selectSegment(int index)
{
    if (index < 0 || index >= m_pipeSegments.size()) {
        return;
    }

    // Если кликаем на тот же сегмент - просто показываем/скрываем информацию
    if (index == m_selectedIndex) {
        toggleInfo();
        return;
    }

    // Снимаем выделение с предыдущего сегмента
    if (m_selectedIndex >= 0 && m_selectedIndex < m_pipeSegments.size()) {
        updateHighlight(m_selectedIndex, false);
    }

    // Устанавливаем новый выбранный сегмент
    m_selectedIndex = index;

    // Добавляем подсветку
    updateHighlight(index, true);

}

void Interaction::selectNextSegment()
{
    if (m_pipeSegments.isEmpty()) {
        return;
    }

    int nextIndex = m_selectedIndex + 1;
    if (nextIndex >= m_pipeSegments.size()) {
        nextIndex = 0; // Зацикливаем
    }

    selectSegment(nextIndex);
}

void Interaction::selectPrevSegment()
{
    if (m_pipeSegments.isEmpty()) {
        return;
    }

    int prevIndex = m_selectedIndex - 1;
    if (prevIndex < 0) {
        prevIndex = m_pipeSegments.size() - 1; // Зацикливаем
    }

    selectSegment(prevIndex);
}

void Interaction::toggleInfo()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_pipeSegments.size()) {
        return;
    }

    if (m_infoText && m_infoText->scene()) {
        // Информация видна - скрываем
        hideSegmentInfo();
    } else {
        // Информации нет - показываем
        showSegmentInfo(m_selectedIndex);
    }
}
