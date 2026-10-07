#pragma once

#include "launcher_config.h"
#include "overlay_manager.h"
#include "qemu_process_controller.h"

#include <QMainWindow>

class QCloseEvent;
class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

namespace launcher {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void browseQemuBinary();
    void browseQemuImgBinary();
    void browseBaseImage();
    void browseOverlayImage();
    void startQemu();
    void stopQemu();
    void clearLog();
    void appendLog(const QString &text);
    void handleProcessStarted();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(const QString &message);
    void handleBackendEvidence(bool ready);

private:
    LauncherConfig currentConfig() const;
    void setRunningUi(bool running);
    void setStatus(const QString &text);
    ExistingOverlayPolicy chooseOverlayPolicy(bool *cancelled);

    QGroupBox *m_configGroup = nullptr;
    QLineEdit *m_qemuBinary = nullptr;
    QLineEdit *m_qemuImgBinary = nullptr;
    QLineEdit *m_baseImage = nullptr;
    QLineEdit *m_overlayImage = nullptr;
    QSpinBox *m_cpuCount = nullptr;
    QSpinBox *m_memoryMiB = nullptr;
    QCheckBox *m_vulkanPreset = nullptr;
    QPlainTextEdit *m_extraArguments = nullptr;
    QPlainTextEdit *m_commandSummary = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_backendStatus = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QString m_lastAutoOverlay;
    QemuProcessController m_controller;
};

} // namespace launcher
