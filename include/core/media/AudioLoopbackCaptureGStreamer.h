#pragma once

#include <QByteArray>
#include <QString>

namespace AudioLoopbackGst {

bool start(const QString &sinkNodeId);
void stop();
bool isRunning();
/// Pulls available float32 stereo PCM at 44.1kHz (may return false if no data ready).
bool pull(QByteArray &pcmOut);

} // namespace AudioLoopbackGst
