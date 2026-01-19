#include "oildrop.h"
#include <QDebug>
#include <QGraphicsScene>

OilDrop::OilDrop(QGraphicsItem *parent)
    : QObject(nullptr), QGraphicsEllipseItem(parent)
    , m_moveTimer(nullptr)
    , m_speed(100.0)
    , m_currentSegment(0)
{
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &OilDrop::updateMovement);
}

OilDrop::~OilDrop()
{
    if (m_moveTimer) {
        m_moveTimer->stop();
        delete m_moveTimer;
    }
}

void OilDrop::setPipeSegments(const QVector<PipeSegmentInfo>& segments)
{
    m_pipeSegments = segments;
}

void OilDrop::startMoving(qreal speed)
{
    m_speed = speed;
    m_currentSegment = 0;

    qDebug() << "OilDrop: starting movement with speed" << m_speed << "px/sec";
    m_moveTimer->start(16);
}

void OilDrop::stopMoving()
{
    if (m_moveTimer) {
        m_moveTimer->stop();
    }
}

void OilDrop::updateMovement()
{
    if (m_pipeSegments.isEmpty()) {
        return;
    }

    QPointF currentPos = pos();
    QRectF currentRect = rect();
    qreal currentX = currentPos.x() + currentRect.width() / 2;

    qreal deltaX = m_speed / 60.0;
    QPointF newPos(currentPos.x() + deltaX, currentPos.y());

    int segmentIndex = -1;
    for (int i = 0; i < m_pipeSegments.size(); i++) {
        const PipeSegmentInfo& segment = m_pipeSegments[i];
        if (currentX >= segment.leftX && currentX <= segment.rightX) {
            segmentIndex = i;
            break;
        }
    }

    if (segmentIndex == -1) {
        if (currentX < m_pipeSegments.first().leftX) {
            segmentIndex = 0;
        } else if (currentX > m_pipeSegments.last().rightX) {
            segmentIndex = m_pipeSegments.size() - 1;
        } else {
            for (int i = 0; i < m_pipeSegments.size() - 1; i++) {
                if (currentX > m_pipeSegments[i].rightX &&
                    currentX < m_pipeSegments[i + 1].leftX) {
                    segmentIndex = i;
                    break;
                }
            }
        }
    }

    if (segmentIndex >= 0 && segmentIndex < m_pipeSegments.size()) {
        const PipeSegmentInfo& segment = m_pipeSegments[segmentIndex];

        qreal targetSize = qMin(segment.innerRect.width(),
                                segment.innerRect.height()) * 0.7;

        if (targetSize < 8) targetSize = 8;
        if (targetSize > 30) targetSize = 30;

        qreal currentSize = currentRect.width();
        if (qAbs(currentSize - targetSize) > 0.5) {
            qreal newSize = currentSize + (targetSize - currentSize) * 0.2;
            setRect(0, 0, newSize, newSize);

            qreal targetY = segment.innerRect.center().y() - newSize / 2;
            newPos.setY(targetY);
        }

        if (segmentIndex != m_currentSegment) {
            m_currentSegment = segmentIndex;
            qreal pipeSize = qMin(segment.innerRect.width(), segment.innerRect.height());
            m_speed = 60.0 + (pipeSize * 0.8);

            qDebug() << "OilDrop: entered segment" << segmentIndex
                     << ", new speed:" << m_speed;
        }
    }

    setPos(newPos);

    const PipeSegmentInfo& lastSegment = m_pipeSegments.last();
    if (currentX > lastSegment.rightX + 100) {
        qDebug() << "OilDrop: finished journey, removing";

        m_moveTimer->stop();

        if (scene()) {
            scene()->removeItem(this);
        }

        deleteLater();
    }
}
