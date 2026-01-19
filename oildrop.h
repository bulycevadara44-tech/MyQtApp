#ifndef OILDROP_H
#define OILDROP_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QRectF>
#include <QVector>

struct PipeSegmentInfo {
    QRectF innerRect;
    int segmentIndex;
    qreal leftX;
    qreal rightX;
};

class OilDrop : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT

public:
    explicit OilDrop(QGraphicsItem *parent = nullptr);
    ~OilDrop();

    void setPipeSegments(const QVector<PipeSegmentInfo>& segments);
    void startMoving(qreal speed = 100.0);
    void stopMoving();

public slots:
    void updateMovement();

private:
    QVector<PipeSegmentInfo> m_pipeSegments;
    QTimer *m_moveTimer;
    qreal m_speed;
    int m_currentSegment;
};

#endif // OILDROP_H
