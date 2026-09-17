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
#ifdef Q_OS_WASM
    w.showMaximized();   // fill the browser container instead of a tiny window
#else
    w.show();
#endif
    return app.exec();
}
