#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Prussiadriver");
    a.setWindowIcon(QIcon(":/icon/Prussian.png"));


    MainWindow w;
    w.show();
    return a.exec();
}
