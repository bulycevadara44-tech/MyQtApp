#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QWidget>
#include <QLineEdit>

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget *parent = nullptr);
    QString userName() const;
    void clearFields();

signals:
    void nextRequested(const QString& name);
    void cancelRequested();

private slots:
    void onNextClicked();
    void onCancelClicked();

private:
    QLineEdit *m_nameEdit;
    QString m_storedName;

    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // LOGINPAGE_H
