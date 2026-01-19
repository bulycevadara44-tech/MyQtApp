#ifndef MODESELECTIONPAGE_H
#define MODESELECTIONPAGE_H

#include <QWidget>
#include <QRadioButton>
#include "pipelineparameters.h"

class ModeSelectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ModeSelectionPage(QWidget *parent = nullptr);
    Mode selectedMode() const;
    void clearSelection();

signals:
    void nextRequested();
    void backRequested();

private slots:
    void onNextClicked();
    void onBackClicked();

private:
    QRadioButton *m_mode1Radio;
    QRadioButton *m_mode2Radio;
    Mode m_selectedMode;
};

#endif // MODESELECTIONPAGE_H
