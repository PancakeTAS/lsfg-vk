#pragma once

#include <QObject>

namespace lsfgvk::ui {

    class UI : public QObject {
        Q_OBJECT

    public:
        explicit UI(QObject* parent = nullptr);
    private:

    };

}
