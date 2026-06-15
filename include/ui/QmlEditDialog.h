#pragma once
#include <QDialog>
#include <memory>

namespace Ui { class QmlEditDialog; }
class DynamicInterfaceSource;

class QmlEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit QmlEditDialog(const QString &initialCode = QString(),
                           QWidget *parent = nullptr);
    ~QmlEditDialog() override;

    QString resultCode() const;

private slots:
    void onPresetSelected(int row);
    void onRefreshPreview();

private:
    void updatePreview();

    Ui::QmlEditDialog                    *ui;
    std::unique_ptr<DynamicInterfaceSource> m_previewSrc;
};
