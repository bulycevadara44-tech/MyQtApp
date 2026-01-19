#ifndef INTERACTION_H
#define INTERACTION_H

#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include "pipelineparameters.h"

class Interaction : public QObject
{
    Q_OBJECT

public:
    explicit Interaction(QGraphicsScene* scene, QObject* parent = nullptr);
    ~Interaction();

    void setup(const QVector<PipeSegmentInfo>& pipeSegments,
               const QVector<double>& diameters,
               const QVector<ValidationResult>& validationResults);

    void clear();

    // Методы для клавиатурного управления
    void selectSegment(int index);
    void selectNextSegment();
    void selectPrevSegment();
    void toggleInfo();

    // Геттеры
    int selectedIndex() const { return m_selectedIndex; }
    int segmentsCount() const { return m_pipeSegments.size(); }

public slots:
    void onSegmentHoverEnter(int index);
    void onSegmentHoverLeave(int index);
    void onSegmentClicked(int index);

private:

    void updateHighlight(int index, bool show);
    void showSegmentInfo(int index);
    void hideSegmentInfo();
    QColor getSegmentColor(int index) const;

    QGraphicsScene* m_scene;
    QVector<PipeSegmentInfo> m_pipeSegments;
    QVector<double> m_diameters;
    QVector<ValidationResult> m_validationResults;
    QVector<QGraphicsRectItem*> m_hitAreas;

    QGraphicsRectItem* m_highlight;
    QGraphicsTextItem* m_infoText;
    QGraphicsRectItem* m_textBackground;

    int m_hoveredIndex;
    int m_selectedIndex;
};

#endif // INTERACTION_H
