#ifndef INPUTPARAMETERSPAGE_H
#define INPUTPARAMETERSPAGE_H

#include <QWidget>
#include <QVector>
#include "pipelineparameters.h"

class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QFormLayout;
class QVBoxLayout;


class InputParametersPage : public QWidget {
    Q_OBJECT

public:
    explicit InputParametersPage(QWidget *parent = nullptr);

    PipelineParameters toParameters() const;
    void setMode(Mode mode);


signals:
    void nextRequested();
    void backRequested();

public slots:
    void clearFields();
private slots:
    void onNextClicked();
    void onBackClicked();

private:
    void setupForm(); // Будет вызвана только один раз
    QVector<double> outerDiameters() const;

    Mode m_mode;

    // Общие поля (всегда видны)
    QDoubleSpinBox *m_pressure;
    QDoubleSpinBox *m_massFlow;
    QDoubleSpinBox *m_operationalFactor;
    QDoubleSpinBox *m_reliabilityYield;
    QDoubleSpinBox *m_reliabilityStrength;
    QDoubleSpinBox *m_responsibilityFactor;
    QDoubleSpinBox *m_pressureReliability;
    QLineEdit *m_outerDiameters;

    // Поля Mode2 (будут скрыты/показаны)
    QDoubleSpinBox *m_density;
    QDoubleSpinBox *m_yieldStrength;
    QDoubleSpinBox *m_tensileStrength;
    QDoubleSpinBox *m_fluidBulkModulus;
    QDoubleSpinBox *m_steelYoungModulus;
    QDoubleSpinBox *m_temperatureDelta;
    QDoubleSpinBox *m_poissonRatio;
    QDoubleSpinBox *m_thermalExpansionCoeff;
    QDoubleSpinBox *m_bendRadius;

    QVBoxLayout *m_mainLayout;
    QFormLayout *m_formLayout;
};

#endif // INPUTPARAMETERSPAGE_H
