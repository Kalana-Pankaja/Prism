#pragma once

#include <QDialog>
#include "ui/ClipNodeEditor.h"

class QWidget;

class GroupEditorDialog : public QDialog {
    Q_OBJECT

public:
    GroupEditorDialog(NodeId groupId, ClipNodeEditor *editor, QWidget *parent = nullptr);

private:
    NodeId m_groupId;
    ClipNodeEditor *m_editor;
    QWidget *m_view = nullptr;
};
