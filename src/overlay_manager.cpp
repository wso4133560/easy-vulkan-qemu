#include "overlay_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace launcher {

OverlayResult ensureOverlay(const LauncherConfig &config, ExistingOverlayPolicy policy)
{
    OverlayResult result;
    const QFileInfo baseInfo(config.baseImage);
    const QFileInfo overlayInfo(config.overlayImage);
    if (!baseInfo.isFile()) {
        result.message = QStringLiteral("基础镜像不存在：") + config.baseImage;
        return result;
    }
    if (overlayInfo.absoluteFilePath().compare(baseInfo.absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        result.message = QStringLiteral("拒绝将基础镜像作为 overlay 覆盖");
        return result;
    }

    if (overlayInfo.exists()) {
        if (policy == ExistingOverlayPolicy::Reuse) {
            result.ok = true;
            result.message = QStringLiteral("复用已有 overlay：") + config.overlayImage;
            return result;
        }
        if (!QFile::remove(config.overlayImage)) {
            result.message = QStringLiteral("无法删除已有 overlay：") + config.overlayImage;
            return result;
        }
    }

    const QFileInfo outputInfo(config.overlayImage);
    if (!outputInfo.absolutePath().isEmpty() && !QDir().mkpath(outputInfo.absolutePath())) {
        result.message = QStringLiteral("无法创建 overlay 目录：") + outputInfo.absolutePath();
        return result;
    }

    QProcess process;
    process.setProgram(config.qemuImgBinary);
    process.setArguments(buildOverlayCreateArguments(config));
    process.start();
    if (!process.waitForStarted(5000)) {
        result.message = QStringLiteral("无法启动 qemu-img：") + process.errorString();
        return result;
    }
    if (!process.waitForFinished(60000)) {
        process.kill();
        process.waitForFinished(3000);
        result.message = QStringLiteral("qemu-img 创建 overlay 超时");
        result.output = QString::fromLocal8Bit(process.readAllStandardError());
        return result;
    }

    const QByteArray stdoutData = process.readAllStandardOutput();
    const QByteArray stderrData = process.readAllStandardError();
    result.output = QString::fromLocal8Bit(stdoutData + stderrData);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.message = QStringLiteral("qemu-img 创建 overlay 失败，退出码：")
                         + QString::number(process.exitCode());
        return result;
    }
    if (!QFileInfo::exists(config.overlayImage)) {
        result.message = QStringLiteral("qemu-img 返回成功，但 overlay 文件未生成");
        return result;
    }

    result.ok = true;
    result.created = true;
    result.message = QStringLiteral("已创建 overlay：") + config.overlayImage;
    return result;
}

} // namespace launcher
