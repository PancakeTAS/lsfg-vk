#pragma once

#include <QObject>

namespace lsfgvk::ui {

    class Backend : public QObject {
        Q_OBJECT

    public:
        explicit Backend(QObject* parent = nullptr);
    private:

    };

}
