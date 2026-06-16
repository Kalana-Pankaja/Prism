#include "ui/OutputWindow.h"
#include "ui_OutputWindow.h"
#include "ui/VideoWidget.h"
#include <QKeyEvent>

OutputWindow::OutputWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::OutputWindow) {
    ui->setupUi(this);
    connect(ui->fullscreenBtn, &QPushButton::clicked, this, &OutputWindow::onFullscreenClicked);
}

OutputWindow::~OutputWindow() {
    delete ui;
}

VideoWidget *OutputWindow::videoWidget() const {
    return ui->outputWidget;
}

void OutputWindow::onFullscreenClicked() {
    if (isFullScreen()) {
        showNormal();
        ui->fullscreenBtn->setText("🖵");
    } else {
        showFullScreen();
        ui->fullscreenBtn->setText("🖦");
    }
}

void OutputWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        showNormal();
        ui->fullscreenBtn->setText("🖵");
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}
