#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "HexBackend.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("nanamitm");
    app.setApplicationName("DailyHexPuzzleAndroid");

    qmlRegisterType<HexBackend>("DailyHexPuzzle", 1, 0, "HexBackend");

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/DailyHexPuzzle/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

#if defined(Q_OS_ANDROID)
    // Keep the screen on while the app is in the foreground.
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (!activity.isValid()) return;
        QJniObject window = activity.callObjectMethod(
            "getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) return;
        // WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON = 0x00000080
        window.callMethod<void>("addFlags", "(I)V", 0x00000080);
    });
#endif

    return app.exec();
}
