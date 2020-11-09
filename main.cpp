#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
    QPushButton btn("hello world");
    a.connect(&btn, SIGNAL(clicked()), &a ,SLOT(quit()));
    btn.show();
    return a.exec();
}
