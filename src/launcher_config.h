#pragma once

#include <QString>
#include <QStringList>

namespace launcher {

struct LauncherConfig {
    QString qemuBinary;
    QString qemuImgBinary;
    QString baseImage;
    QString overlayImage;
    int cpuCount = 2;
    int memoryMiB = 2048;
    bool vulkanPreset = true;
    QStringList extraArguments;
};

struct ValidationResult {
    bool ok = false;
    QStringList errors;

    QString summary() const;
};

QStringList parseExtraArguments(const QString &text);
ValidationResult validateConfig(const LauncherConfig &config);
QStringList buildQemuArguments(const LauncherConfig &config);
QStringList buildOverlayCreateArguments(const LauncherConfig &config);
QString commandSummary(const QString &program, const QStringList &arguments);
QString defaultOverlayForBase(const QString &baseImage);

} // namespace launcher
