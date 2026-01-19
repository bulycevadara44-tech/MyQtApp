#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include <QWidget>
#include <QVector>
#include <QRectF>
#include <QPixmap>
#include <QGraphicsEllipseItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>
#include "pipelineparameters.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QElapsedTimer>

class Interaction;

class ResultPage : public QWidget
{
    Q_OBJECT

public:
    explicit ResultPage(QWidget *parent = nullptr);
    ~ResultPage();

    void setResults(const QString &optimalDiameter, const QString &optimalThickness,
                    const QString &safetyHoop, const QString &safetyAxial,
                    const QString &safetyEquivalent, const QString &minSafety,
                    const QVector<double>& diameters, const QPixmap &backgroundImage,
                    const QVector<ValidationResult>& validationResults);

    void clearPage();

    void saveCalculationsToTxt();
    void saveImageToPng();
    void setUserData(const QString& userName, Mode mode, const PipelineParameters& params);
    void showEditDialog();

    QString getUserName() const { return m_userName; }
    Mode getMode() const { return m_mode; }
    PipelineParameters getParams() const { return m_params; }
    QVector<ValidationResult> getValidationResults() const { return m_validationResults; }

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void restartRequested();
    void exitRequested();

private slots:
    void onRestartClicked();
    void onExitClicked();
    void onSaveCalculationsClicked();
    void onSaveImageClicked();

private:
    QElapsedTimer m_animationElapsedTimer;
    void setupUI();
    void createPipelineVisualization(const QVector<double>& diameters, const QPixmap &backgroundImage);
    void startOilAnimation();
    void updateOilAnimation();
    void createOilDrop();
    void cleanupOilDrops();

    struct SimpleDrop {
        QGraphicsEllipseItem* item = nullptr;
        qreal speed = 0.0;
        int currentSegment = -1;
    };

    // UI элементы
    QLabel *m_resultLabel;
    QGraphicsView *m_graphicsView;
    QPushButton *m_restartBtn;
    QPushButton *m_exitBtn;
    QMenuBar *m_menuBar;
    QMenu *m_saveMenu;
    QAction *m_saveCalculationsAction;
    QAction *m_saveImageAction;

    // Графика
    QGraphicsScene *m_scene;
    Interaction *m_interaction;
    QTimer *m_animationTimer;
    int m_frameCounter;
    QVector<SimpleDrop> m_activeDrops;
    QVector<PipeSegmentInfo> m_pipeSegments;

    // Данные
    QVector<double> m_diameters;
    QVector<ValidationResult> m_validationResults;

    // Для сохранения
    QString m_userName;
    Mode m_mode;
    PipelineParameters m_params;
};
#endif // RESULTPAGE_H
