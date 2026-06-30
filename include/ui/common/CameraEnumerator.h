#pragma once

#include <QString>
#include <QList>

class QWidget;

struct CameraDeviceInfo {
    QString id;
    QString label;
    bool    isDefault = false;
};

namespace CameraEnumerator {

/// Lists camera devices for UI pickers (Qt Multimedia + platform extras).
QList<CameraDeviceInfo> listDevices(QWidget *warmupParent = nullptr);

} // namespace CameraEnumerator
