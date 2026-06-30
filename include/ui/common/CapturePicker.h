#pragma once

#include "core/sources/SourceDescriptor.h"

class QWidget;

namespace CapturePicker {

/// Screen/window picker (monitor list + capturable window list). Not used on Linux
/// (portal handles capture there).
bool prompt(QWidget *parent, SourceDescriptor &desc,
            SourceDescriptor::Kind preferredKind);

} // namespace CapturePicker
