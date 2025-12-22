#include "ui.hpp"

#include <QObject>
#include <iostream>

using namespace lsfgvk::ui;

UI::UI(QObject* parent) : QObject(parent) {
    std::cerr << "Hello, world!" << '\n';
}
