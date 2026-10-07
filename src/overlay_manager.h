#pragma once

#include "launcher_config.h"

#include <QString>

namespace launcher {

enum class ExistingOverlayPolicy {
    Reuse,
    Recreate,
};

struct OverlayResult {
    bool ok = false;
    bool created = false;
    QString message;
    QString output;
};

OverlayResult ensureOverlay(const LauncherConfig &config, ExistingOverlayPolicy policy);

} // namespace launcher
