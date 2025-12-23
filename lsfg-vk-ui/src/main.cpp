#include "backend.hpp"

#include <QIcon>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

using namespace lsfgvk::ui;

int main(int argc, char* argv[]) {
    const QGuiApplication app(argc, argv);
    QGuiApplication::setWindowIcon(QIcon(":/rsc/icon.png"));
    QGuiApplication::setApplicationName("lsfg-vk-ui");
    QGuiApplication::setApplicationDisplayName("lsfg-vk-ui");

    QQmlApplicationEngine engine;
    Backend backend;

    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load("qrc:/rsc/UI.qml");

    return QGuiApplication::exec();
}
