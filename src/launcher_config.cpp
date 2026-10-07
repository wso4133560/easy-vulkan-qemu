#include "launcher_config.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace launcher {

namespace {

const QStringList kReservedArguments = {
    QStringLiteral("-accel"),
    QStringLiteral("-device"),
    QStringLiteral("-display"),
    QStringLiteral("-drive"),
    QStringLiteral("-m"),
    QStringLiteral("-smp"),
};

bool isReservedArgument(const QString &argument)
{
    const QString lower = argument.trimmed().toLower();
    return kReservedArguments.contains(lower);
}

QString quoteForDisplay(const QString &value)
{
    if (value.isEmpty()) {
        return QStringLiteral("\"\"");
    }
    if (!value.contains(QRegularExpression(QStringLiteral("[\\s\"]")))) {
        return value;
    }
    QString escaped = value;
    escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

} // namespace

QString ValidationResult::summary() const
{
    return errors.join(QStringLiteral("\n"));
}

QStringList parseExtraArguments(const QString &text)
{
    QStringList arguments;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                         Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString argument = line.trimmed();
        if (!argument.isEmpty()) {
            arguments.append(argument);
        }
    }
    return arguments;
}

ValidationResult validateConfig(const LauncherConfig &config)
{
    ValidationResult result;
    const auto requireFile = [&result](const QString &path, const QString &label) {
        if (path.trimmed().isEmpty()) {
            result.errors.append(label + QStringLiteral("不能为空"));
        } else if (!QFileInfo(path).isFile()) {
            result.errors.append(label + QStringLiteral("不存在：") + path);
        }
    };

    requireFile(config.qemuBinary, QStringLiteral("QEMU system binary"));
    requireFile(config.qemuImgBinary, QStringLiteral("qemu-img"));
    requireFile(config.baseImage, QStringLiteral("guest 基础镜像"));

    if (config.overlayImage.trimmed().isEmpty()) {
        result.errors.append(QStringLiteral("guest overlay 路径不能为空"));
    }
    if (config.cpuCount <= 0) {
        result.errors.append(QStringLiteral("CPU 数量必须大于 0"));
    }
    if (config.memoryMiB <= 0) {
        result.errors.append(QStringLiteral("内存必须大于 0 MiB"));
    }

    for (const QString &argument : config.extraArguments) {
        if (isReservedArgument(argument)) {
            result.errors.append(QStringLiteral("额外参数不能覆盖核心参数：") + argument);
        }
    }

    const QString base = QFileInfo(config.baseImage).absoluteFilePath();
    const QString overlay = QFileInfo(config.overlayImage).absoluteFilePath();
    if (!base.isEmpty() && base.compare(overlay, Qt::CaseInsensitive) == 0) {
        result.errors.append(QStringLiteral("基础镜像和 overlay 不能是同一个文件"));
    }

    result.ok = result.errors.isEmpty();
    return result;
}

QStringList buildQemuArguments(const LauncherConfig &config)
{
    QStringList arguments;
    arguments << QStringLiteral("-accel") << QStringLiteral("whpx");
    arguments << QStringLiteral("-m") << QString::number(config.memoryMiB);
    arguments << QStringLiteral("-smp") << QString::number(config.cpuCount);
    arguments << QStringLiteral("-drive")
              << QStringLiteral("file=%1,if=virtio,format=qcow2")
                     .arg(QDir::toNativeSeparators(config.overlayImage));

    if (config.vulkanPreset) {
        arguments << QStringLiteral("-display") << QStringLiteral("none");
        arguments << QStringLiteral("-device")
                  << QStringLiteral("virtio-gpu-rutabaga-pci,gfxstream-vulkan=on,wsi=surfaceless,hostmem=512M");
    }

    arguments.append(config.extraArguments);
    return arguments;
}

QStringList buildOverlayCreateArguments(const LauncherConfig &config)
{
    return {
        QStringLiteral("create"),
        QStringLiteral("-f"),
        QStringLiteral("qcow2"),
        QStringLiteral("-F"),
        QStringLiteral("qcow2"),
        QStringLiteral("-b"),
        config.baseImage,
        config.overlayImage,
    };
}

QString commandSummary(const QString &program, const QStringList &arguments)
{
    QStringList parts;
    parts.append(quoteForDisplay(program));
    for (const QString &argument : arguments) {
        parts.append(quoteForDisplay(argument));
    }
    return parts.join(QLatin1Char(' '));
}

QString defaultOverlayForBase(const QString &baseImage)
{
    if (baseImage.trimmed().isEmpty()) {
        return {};
    }
    return QFileInfo(baseImage).absoluteFilePath() + QStringLiteral(".overlay.qcow2");
}

} // namespace launcher
