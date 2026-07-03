#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QLocale>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    // Wayland nativo, com fallback para X11 se não houver compositor.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "wayland;xcb");

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("tracker-horas"));
    app.setApplicationDisplayName(QStringLiteral("Tracker Horas"));
    app.setDesktopFileName(QStringLiteral("tracker-horas"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

    QFile qss(QStringLiteral(":/style.qss"));
    if (qss.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow window;
    window.show();
    return app.exec();
}
