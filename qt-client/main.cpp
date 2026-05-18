#include <QApplication>
#include "mainwindow.h"
#include "appicon.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("nanamitm");
    app.setApplicationName("DailyHexPuzzle");
    app.setWindowIcon(makeHexIcon());

    MainWindow w;
    w.show();
    return app.exec();
}
