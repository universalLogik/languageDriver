#include "mainwindow.h"

#include <QApplication>
#include <csignal>

int main(int argc, char *argv[])
{
#ifndef Q_OS_WIN
    signal(SIGPIPE, SIG_IGN);
#endif

    QApplication a(argc, argv);
    a.setApplicationName("languageDriver");
    a.setWindowIcon(QIcon(":/icon/languageDriver.png"));


    MainWindow w;
    w.show();
    return a.exec();
}
