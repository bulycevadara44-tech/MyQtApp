#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMessageBox>
#include "loginpage.h"
#include "modeselectionpage.h"
#include "inputparameterspage.h"
#include "resultpage.h"
#include "pipelineoptimizer.h"
#include "pipelineparameters.h"

class MainClass : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainClass(QWidget *parent = nullptr);
    ~MainClass();
    void centerWindow();

private slots:
    void onLoginNext();
    void onLoginCancel();
    void onModeNext();
    void onModeBack();
    void onInputNext();
    void onInputBack();
    void onResultRestart();
    void onResultExit();


private:
    QStackedWidget *m_stackedWidget;
    LoginPage *m_loginPage;
    ModeSelectionPage *m_modePage;
    InputParametersPage *m_inputPage;
    ResultPage *m_resultPage;

    QString m_userName;
    Mode m_selectedMode;
    PipelineParameters m_currentParams;
};

#endif // MAINCLASS_H
