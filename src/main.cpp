#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
#ifdef Q_OS_LINUX
    // Wayland nativo, com fallback para X11 se não houver compositor.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "wayland;xcb");
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("tracker-horas"));
    app.setApplicationDisplayName(QStringLiteral("Tracker Horas"));
    app.setDesktopFileName(QStringLiteral("tracker-horas"));
    app.setWindowIcon(QIcon(QStringLiteral(":/tracker-horas.svg")));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

    Theme::apply(&app, Theme::savedMode());

    MainWindow window;
    window.show();
    return app.exec();
}
