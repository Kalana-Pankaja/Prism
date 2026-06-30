#include "ui/common/CapturePicker.h"

#include "core/sources/WindowCaptureSource.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace CapturePicker {

bool prompt(QWidget *parent, SourceDescriptor &desc, SourceDescriptor::Kind preferredKind) {
    const auto screens = QGuiApplication::screens();
    QList<QCapturableWindow> windows = WindowCaptureSource::capturableWindows();

    if (screens.isEmpty() && windows.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("No Capture Sources"),
                             QObject::tr("No screens or capturable windows were found."));
        return false;
    }

    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("Select Capture Source"));
    dlg.setMinimumWidth(420);

    auto *monitorRadio = new QRadioButton(QObject::tr("Entire screen"), &dlg);
    auto *windowRadio  = new QRadioButton(QObject::tr("Application window"), &dlg);
    monitorRadio->setEnabled(!screens.isEmpty());
    windowRadio->setEnabled(!windows.isEmpty());

    const bool preferWindow = preferredKind == SourceDescriptor::Kind::Window
                              && windowRadio->isEnabled();
    if (preferWindow)
        windowRadio->setChecked(true);
    else
        monitorRadio->setChecked(true);

    auto *stack = new QStackedWidget(&dlg);

    auto *screenCombo = new QComboBox(&dlg);
    for (int i = 0; i < screens.size(); ++i) {
        screenCombo->addItem(
            QObject::tr("Screen %1 — %2").arg(i + 1).arg(screens[i]->name()));
    }
    screenCombo->setCurrentIndex(qBound(0, desc.screenIndex, screens.size() - 1));

    auto *windowPage = new QWidget(&dlg);
    auto *windowLayout = new QVBoxLayout(windowPage);
    windowLayout->setContentsMargins(0, 0, 0, 0);

    auto *windowCombo = new QComboBox(windowPage);
    auto fillWindows = [&]() {
        const int previous = windowCombo->currentIndex();
        windowCombo->clear();
        windows = WindowCaptureSource::capturableWindows();
        for (const auto &w : windows) {
            windowCombo->addItem(w.description().isEmpty()
                                     ? QObject::tr("(unnamed window)")
                                     : w.description());
        }
        if (!windows.isEmpty())
            windowCombo->setCurrentIndex(qBound(0, desc.windowIndex, windows.size() - 1));
        else if (previous >= 0)
            windowCombo->setCurrentIndex(qBound(0, previous, windowCombo->count() - 1));
        windowRadio->setEnabled(!windows.isEmpty());
    };
    fillWindows();

    auto *refreshBtn = new QPushButton(QObject::tr("Refresh window list"), windowPage);
    QObject::connect(refreshBtn, &QPushButton::clicked, &dlg, fillWindows);
    windowLayout->addWidget(windowCombo);
    windowLayout->addWidget(refreshBtn);

    stack->addWidget(screenCombo);
    stack->addWidget(windowPage);
    stack->setCurrentIndex(windowRadio->isChecked() ? 1 : 0);

    QObject::connect(monitorRadio, &QRadioButton::toggled, &dlg, [&](bool on) {
        if (on)
            stack->setCurrentIndex(0);
    });
    QObject::connect(windowRadio, &QRadioButton::toggled, &dlg, [&](bool on) {
        if (on)
            stack->setCurrentIndex(1);
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QObject::tr("What do you want to capture?"), &dlg));
    layout->addWidget(monitorRadio);
    layout->addWidget(windowRadio);
    layout->addWidget(stack);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    if (monitorRadio->isChecked()) {
        if (screens.isEmpty())
            return false;
        const int idx = screenCombo->currentIndex();
        desc.kind         = SourceDescriptor::Kind::Screen;
        desc.screenIndex  = idx;
        desc.displayName  = QObject::tr("Screen %1").arg(idx + 1);
    } else {
        if (windows.isEmpty())
            return false;
        const int idx = qBound(0, windowCombo->currentIndex(), windows.size() - 1);
        desc.kind         = SourceDescriptor::Kind::Window;
        desc.windowIndex  = idx;
        desc.displayName  = windowCombo->itemText(idx);
    }
    return true;
}

} // namespace CapturePicker
