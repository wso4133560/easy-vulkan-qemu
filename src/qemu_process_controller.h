#pragma once

#include <QProcess>
#include <QObject>

namespace launcher {

class QemuProcessController final : public QObject {
    Q_OBJECT

public:
    explicit QemuProcessController(QObject *parent = nullptr);

    bool isRunning() const;
    bool gfxstreamInitialized() const;
    bool vulkanDeviceCreated() const;
    QString lastError() const;
    int lastExitCode() const;

public slots:
    void start(const QString &program, const QStringList &arguments);
    void stop();

signals:
    void logReceived(const QString &text);
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void processError(const QString &message);
    void backendEvidenceChanged(bool ready);

private slots:
    void readStandardOutput();
    void readStandardError();
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleError(QProcess::ProcessError error);

private:
    void consumeOutput(const QByteArray &data);

    QProcess m_process;
    bool m_gfxstreamInitialized = false;
    bool m_vulkanDeviceCreated = false;
    QString m_lastError;
    int m_lastExitCode = -1;
};

} // namespace launcher
