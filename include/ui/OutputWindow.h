#pragma once

#include <QMainWindow>

namespace Ui { class OutputWindow; }
class VideoWidget;

class OutputWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent = nullptr);
    ~OutputWindow();

    VideoWidget *videoWidget() const;

private:
    Ui::OutputWindow *ui;

private slots:
    void onFullscreenClicked();
};
