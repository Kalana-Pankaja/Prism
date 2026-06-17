#include "ui/MirrorOutputWindow.h"
#include "ui/ProgramMirrorWidget.h"
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

MirrorOutputWindow::MirrorOutputWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SwitchX - Preview Output"));
    resize(800, 600);

    auto *central = new QWidget(this);
    auto *layout  = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_mirror = new ProgramMirrorWidget(central);
    m_mirror->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_mirror, 1);

    auto *controls = new QHBoxLayout();
    controls->setContentsMargins(8, 8, 8, 8);
    controls->addStretch();

    m_fullscreenBtn = new QPushButton(QStringLiteral("🖵"), central);
    m_fullscreenBtn->setMaximumWidth(50);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &MirrorOutputWindow::onFullscreenClicked);
    controls->addWidget(m_fullscreenBtn);

    layout->addLayout(controls);
    setCentralWidget(central);
}

void MirrorOutputWindow::setFrame(const QImage &frame) {
    if (m_mirror)
        m_mirror->setFrame(frame);
}

void MirrorOutputWindow::onFullscreenClicked() {
    if (isFullScreen()) {
        showNormal();
        m_fullscreenBtn->setText(QStringLiteral("🖵"));
    } else {
        showFullScreen();
        m_fullscreenBtn->setText(QStringLiteral("🖦"));
    }
}

void MirrorOutputWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        showNormal();
        m_fullscreenBtn->setText(QStringLiteral("🖵"));
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}
