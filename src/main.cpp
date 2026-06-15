#include <QApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication app(argc, argv);
    app.setStyle("fusion");

    MainWindow window;
    window.show();

    return app.exec();
}
