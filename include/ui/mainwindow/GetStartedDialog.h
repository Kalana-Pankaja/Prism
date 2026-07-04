#pragma once

#include <QDialog>

class QWidget;

/// First-launch welcome dialog with documentation and support links.
class GetStartedDialog : public QDialog {
    Q_OBJECT
public:
    explicit GetStartedDialog(QWidget *parent = nullptr);

    static bool shouldShow();
    static void markShown();
    static void showIfNeeded(QWidget *parent);
};
