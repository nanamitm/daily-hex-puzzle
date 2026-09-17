#include <QApplication>
#include <QScreen>
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
    // Fill the browser container, and keep filling it: Qt for WebAssembly does
    // not resize a maximized window when the browser window changes size.
    if (QScreen* screen = app.primaryScreen()) {
        w.setGeometry(screen->geometry());
        QObject::connect(screen, &QScreen::geometryChanged,
                         &w, [&w](const QRect& g) { w.setGeometry(g); });
    }
    w.show();
#else
    w.show();
#endif
    return app.exec();
}
