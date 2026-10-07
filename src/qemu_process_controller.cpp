#include "qemu_process_controller.h"

#include <QRegularExpression>

namespace launcher {

QemuProcessController::QemuProcessController(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &QemuProcessController::readStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &QemuProcessController::readStandardError);
    connect(&m_process, &QProcess::started,
            this, &QemuProcessController::handleStarted);
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &QemuProcessController::handleFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &QemuProcessController::handleError);
}

bool QemuProcessController::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

bool QemuProcessController::gfxstreamInitialized() const
{
    return m_gfxstreamInitialized;
}

bool QemuProcessController::vulkanDeviceCreated() const
{
    return m_vulkanDeviceCreated;
}

QString QemuProcessController::lastError() const
{
    return m_lastError;
}

int QemuProcessController::lastExitCode() const
{
    return m_lastExitCode;
}

void QemuProcessController::start(const QString &program, const QStringList &arguments)
{
    if (isRunning()) {
        emit processError(QStringLiteral("QEMU 已在运行，拒绝重复启动"));
        return;
    }

    m_lastError.clear();
    m_lastExitCode = -1;
    m_gfxstreamInitialized = false;
    m_vulkanDeviceCreated = false;
    m_process.setProgram(program);
    m_process.setArguments(arguments);
    emit logReceived(QStringLiteral("启动：") + program + QStringLiteral("\n"));
    m_process.start();
}

void QemuProcessController::stop()
{
    if (!isRunning()) {
        return;
    }
    emit logReceived(QStringLiteral("请求停止 QEMU...\n"));
    m_process.terminate();
    if (!m_process.waitForFinished(3000)) {
        emit logReceived(QStringLiteral("QEMU 未在 3 秒内退出，执行强制终止。\n"));
        m_process.kill();
        m_process.waitForFinished(3000);
    }
}

void QemuProcessController::readStandardOutput()
{
    consumeOutput(m_process.readAllStandardOutput());
}

void QemuProcessController::readStandardError()
{
    consumeOutput(m_process.readAllStandardError());
}

void QemuProcessController::handleStarted()
{
    emit processStarted();
}

void QemuProcessController::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    m_lastExitCode = exitCode;
    emit processFinished(exitCode, status);
}

void QemuProcessController::handleError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    m_lastError = m_process.errorString();
    emit processError(m_lastError);
}

void QemuProcessController::consumeOutput(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    const QString text = QString::fromLocal8Bit(data);
    emit logReceived(text);

    const QString lower = text.toLower();
    const bool gfxstreamSeen = lower.contains(QStringLiteral("gfxstream initialized successfully"));
    const bool vulkanDeviceSeen = lower.contains(QStringLiteral("vulkan"))
                                  && (lower.contains(QStringLiteral("device"))
                                      || lower.contains(QStringLiteral("instance")));
    const bool wasReady = m_gfxstreamInitialized && m_vulkanDeviceCreated;
    m_gfxstreamInitialized = m_gfxstreamInitialized || gfxstreamSeen;
    m_vulkanDeviceCreated = m_vulkanDeviceCreated || vulkanDeviceSeen;
    const bool isReady = m_gfxstreamInitialized && m_vulkanDeviceCreated;
    if (wasReady != isReady) {
        emit backendEvidenceChanged(isReady);
    }
}

} // namespace launcher
