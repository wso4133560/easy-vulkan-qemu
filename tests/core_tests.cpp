#include "launcher_config.h"
#include "overlay_manager.h"
#include "qemu_process_controller.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QTextStream>

using namespace launcher;

namespace {

bool expect(bool condition, const QString &message, QTextStream &out)
{
    if (!condition) {
        out << "FAIL: " << message << Qt::endl;
        return false;
    }
    return true;
}

QString testDirectory(const QString &name)
{
    const QString path = QDir::current().filePath(QStringLiteral("qemu-launcher-core-test-") + name);
    QDir(path).removeRecursively();
    QDir().mkpath(path);
    return path;
}

bool validConfigPasses(QTextStream &out)
{
    const QString directory = testDirectory(QStringLiteral("valid"));
    const QString qemu = QDir(directory).filePath(QStringLiteral("qemu-system-x86_64.exe"));
    const QString qemuImg = QDir(directory).filePath(QStringLiteral("qemu-img.exe"));
    const QString base = QDir(directory).filePath(QStringLiteral("base.qcow2"));
    QFile qemuFile(qemu);
    QFile qemuImgFile(qemuImg);
    QFile baseFile(base);
    bool ok = expect(QDir().mkpath(directory), QStringLiteral("create valid test directory"), out);
    ok = expect(qemuFile.open(QIODevice::WriteOnly), qemuFile.errorString(), out) && ok;
    ok = expect(qemuImgFile.open(QIODevice::WriteOnly), qemuImgFile.errorString(), out) && ok;
    ok = expect(baseFile.open(QIODevice::WriteOnly), baseFile.errorString(), out) && ok;
    qemuFile.close();
    qemuImgFile.close();
    baseFile.close();

    LauncherConfig config;
    config.qemuBinary = qemu;
    config.qemuImgBinary = qemuImg;
    config.baseImage = base;
    config.overlayImage = QDir(directory).filePath(QStringLiteral("run.overlay.qcow2"));
    config.extraArguments = {QStringLiteral("-net"), QStringLiteral("user,id=net0")};
    const ValidationResult result = validateConfig(config);
    ok = expect(result.ok, result.summary(), out) && ok;
    QDir(directory).removeRecursively();
    return ok;
}

bool invalidConfigReportsFields(QTextStream &out)
{
    LauncherConfig config;
    config.qemuBinary = QStringLiteral("missing-qemu.exe");
    config.qemuImgBinary = QStringLiteral("missing-qemu-img.exe");
    config.baseImage = QStringLiteral("missing-base.qcow2");
    config.overlayImage = config.baseImage;
    config.cpuCount = 0;
    config.memoryMiB = 0;
    config.extraArguments = {QStringLiteral("-device"), QStringLiteral("virtio-net")};
    const ValidationResult result = validateConfig(config);
    bool ok = expect(!result.ok, QStringLiteral("invalid config rejected"), out);
    ok = expect(result.summary().contains(QStringLiteral("QEMU system binary")),
                QStringLiteral("missing QEMU is reported"), out) && ok;
    ok = expect(result.summary().contains(QStringLiteral("CPU 数量")),
                QStringLiteral("invalid CPU is reported"), out) && ok;
    ok = expect(result.summary().contains(QStringLiteral("不能覆盖核心参数")),
                QStringLiteral("reserved argument is reported"), out) && ok;
    return ok;
}

bool vulkanArgumentsKeepCoreOptions(QTextStream &out)
{
    LauncherConfig config;
    config.overlayImage = QStringLiteral("C:/images/run.overlay.qcow2");
    config.cpuCount = 4;
    config.memoryMiB = 4096;
    config.extraArguments = {QStringLiteral("-net"), QStringLiteral("user,id=net0")};
    const QStringList arguments = buildQemuArguments(config);
    const QString joined = arguments.join(QLatin1Char(' '));
    bool ok = expect(arguments.contains(QStringLiteral("whpx")), QStringLiteral("WHPX is present"), out);
    ok = expect(joined.contains(QStringLiteral("gfxstream-vulkan=on,wsi=surfaceless,hostmem=512M")),
                QStringLiteral("Vulkan core parameters are present"), out) && ok;
    ok = expect(arguments.contains(QStringLiteral("-net")), QStringLiteral("extra arguments are appended"), out) && ok;
    ok = expect(arguments.at(arguments.indexOf(QStringLiteral("-m")) + 1) == QStringLiteral("4096"),
                QStringLiteral("memory is encoded"), out) && ok;
    return ok;
}

bool overlayArgumentsUseBackingImage(QTextStream &out)
{
    LauncherConfig config;
    config.baseImage = QStringLiteral("C:/images/base.qcow2");
    config.overlayImage = QStringLiteral("C:/images/run.overlay.qcow2");
    const QStringList arguments = buildOverlayCreateArguments(config);
    bool ok = expect(arguments.at(0) == QStringLiteral("create"), QStringLiteral("overlay uses create"), out);
    ok = expect(arguments.contains(QStringLiteral("qcow2")), QStringLiteral("overlay uses qcow2"), out) && ok;
    ok = expect(arguments.at(arguments.indexOf(QStringLiteral("-b")) + 1) == config.baseImage,
                QStringLiteral("backing image is passed"), out) && ok;
    ok = expect(arguments.last() == config.overlayImage, QStringLiteral("overlay output is passed"), out) && ok;
    return ok;
}

bool overlayReuseDoesNotDeleteFile(QTextStream &out)
{
    const QString directory = testDirectory(QStringLiteral("overlay"));
    const QString base = QDir(directory).filePath(QStringLiteral("base.qcow2"));
    const QString overlay = QDir(directory).filePath(QStringLiteral("run.overlay.qcow2"));
    QFile baseFile(base);
    QFile existing(overlay);
    bool ok = expect(baseFile.open(QIODevice::WriteOnly), baseFile.errorString(), out);
    baseFile.close();
    ok = expect(existing.open(QIODevice::WriteOnly), existing.errorString(), out) && ok;
    existing.write("keep");
    existing.close();

    LauncherConfig config;
    config.baseImage = base;
    config.overlayImage = overlay;
    config.qemuImgBinary = QDir(directory).filePath(QStringLiteral("qemu-img.exe"));
    const OverlayResult result = ensureOverlay(config, ExistingOverlayPolicy::Reuse);
    ok = expect(result.ok, result.message, out) && ok;
    ok = expect(!result.created, QStringLiteral("existing overlay is reused"), out) && ok;
    QFile verify(overlay);
    ok = expect(verify.open(QIODevice::ReadOnly), verify.errorString(), out) && ok;
    ok = expect(verify.readAll() == QByteArray("keep"), QStringLiteral("overlay contents are preserved"), out) && ok;
    QDir(directory).removeRecursively();
    return ok;
}

bool controllerRunsCommand(const QStringList &arguments, int expectedExitCode, QTextStream &out)
{
    QemuProcessController controller;
    QEventLoop loop;
    bool started = false;
    bool processError = false;
    int exitCode = -1;
    QObject::connect(&controller, &QemuProcessController::processStarted,
                     [&started] { started = true; });
    QObject::connect(&controller, &QemuProcessController::processError,
                     [&processError] { processError = true; });
    QObject::connect(&controller, &QemuProcessController::processFinished,
                     [&loop, &exitCode](int code, QProcess::ExitStatus) {
                         exitCode = code;
                         loop.quit();
                     });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    QString commandShell = qEnvironmentVariable("ComSpec");
    if (!QFileInfo(commandShell).isFile()) {
        commandShell = QStringLiteral("C:/Windows/System32/cmd.exe");
    }
    controller.start(commandShell, arguments);
    loop.exec();
    if (controller.isRunning()) {
        controller.stop();
    }

    bool ok = expect(started, QStringLiteral("test process started"), out);
    ok = expect(!processError, controller.lastError(), out) && ok;
    ok = expect(exitCode == expectedExitCode,
                QStringLiteral("unexpected test process exit code: %1").arg(exitCode), out) && ok;
    return ok;
}

bool controllerHandlesExitCodes(QTextStream &out)
{
    bool ok = controllerRunsCommand({QStringLiteral("/c"), QStringLiteral("exit 0")}, 0, out);
    ok = controllerRunsCommand({QStringLiteral("/c"), QStringLiteral("exit 7")}, 7, out) && ok;
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    Q_UNUSED(application);
    QTextStream out(stdout);
    int failures = 0;
    failures += !validConfigPasses(out);
    failures += !invalidConfigReportsFields(out);
    failures += !vulkanArgumentsKeepCoreOptions(out);
    failures += !overlayArgumentsUseBackingImage(out);
    failures += !overlayReuseDoesNotDeleteFile(out);
    failures += !controllerHandlesExitCodes(out);
    out << (failures == 0 ? "PASS: all core checks\n" : QString("FAILURES: %1\n").arg(failures));
    return failures == 0 ? 0 : 1;
}
