#include "mainclass.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainClass w;
    qDebug() << "Application started";
    w.show();
    int result = a.exec();
    qDebug() << "Application finished with code:" << result;
    return result;

}
