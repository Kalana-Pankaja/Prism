#pragma once

#include <QDialog>
#include <QString>
#include "core/HtmlWorkspace.h"

namespace Ui { class HtmlEditDialog; }
class QWebEngineView;

class HtmlEditDialog : public QDialog {
    Q_OBJECT

public:
    enum class EditMode { Simple, Workspace };

    explicit HtmlEditDialog(const QString &initialHtml = {},
                            const QString &initialWorkspaceJson = {},
                            QWidget *parent = nullptr);
    ~HtmlEditDialog() override;

    EditMode editMode() const;

    QString resultHtml() const;
    QString resultFilePath() const;
    QString resultWorkspaceJson() const;
    QString resultBakedHtml() const;

private slots:
    void onPresetSelected(int row);
    void onBrowse();
    void onClearFile();
    void onRefreshSimplePreview();
    void onWorkspaceChanged();
    void onComponentSelected(int index);
    void onPropSpinChanged();
    void onDeleteComponent();
    void onDuplicateComponent();
    void onBuildWorkspacePreview();
    void onModeTabChanged(int index);

private:
    void setupPalette();
    void loadSimplePreview(const QString &html, const QString &filePath = {});
    void loadWorkspacePreview(const QString &html);
    void syncPropsFromSelection();
    void applyPropsToSelection();
    HtmlWorkspace currentWorkspace() const;

    Ui::HtmlEditDialog *ui;
    QWebEngineView     *m_simplePreview = nullptr;
    QWebEngineView     *m_wsPreview     = nullptr;
    bool                m_propChanging  = false;
};
