#include "autorunwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    autoRunWindow w;
    w.show();
    return a.exec();
}
