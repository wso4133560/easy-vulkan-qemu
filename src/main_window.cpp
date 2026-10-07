#include "main_window.h"

#include "overlay_manager.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <functional>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>

namespace launcher {

namespace {

QWidget *pathEditor(QWidget *parent, QLineEdit **editor, const QString &buttonText,
                    const std::function<void()> &browse)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    *editor = new QLineEdit(row);
    auto *button = new QPushButton(buttonText, row);
    button->setAutoDefault(false);
    QObject::connect(button, &QPushButton::clicked, row, browse);
    layout->addWidget(*editor, 1);
    layout->addWidget(button);
    return row;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Easy Vulkan QEMU"));
    resize(1050, 760);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    m_configGroup = new QGroupBox(QStringLiteral("QEMU 配置"), central);
    auto *form = new QFormLayout(m_configGroup);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    form->addRow(QStringLiteral("QEMU system binary"),
                 pathEditor(m_configGroup, &m_qemuBinary, QStringLiteral("浏览..."),
                            [this] { browseQemuBinary(); }));
    form->addRow(QStringLiteral("qemu-img"),
                 pathEditor(m_configGroup, &m_qemuImgBinary, QStringLiteral("浏览..."),
                            [this] { browseQemuImgBinary(); }));
    form->addRow(QStringLiteral("guest 基础镜像"),
                 pathEditor(m_configGroup, &m_baseImage, QStringLiteral("浏览..."),
                            [this] { browseBaseImage(); }));
    form->addRow(QStringLiteral("disposable overlay"),
                 pathEditor(m_configGroup, &m_overlayImage, QStringLiteral("浏览..."),
                            [this] { browseOverlayImage(); }));

    auto *resourceRow = new QWidget(m_configGroup);
    auto *resourceLayout = new QHBoxLayout(resourceRow);
    resourceLayout->setContentsMargins(0, 0, 0, 0);
    m_cpuCount = new QSpinBox(resourceRow);
    m_cpuCount->setRange(1, 128);
    m_cpuCount->setValue(2);
    m_cpuCount->setSuffix(QStringLiteral(" vCPU"));
    m_memoryMiB = new QSpinBox(resourceRow);
    m_memoryMiB->setRange(128, 1024 * 1024);
    m_memoryMiB->setValue(2048);
    m_memoryMiB->setSuffix(QStringLiteral(" MiB"));
    resourceLayout->addWidget(new QLabel(QStringLiteral("CPU"), resourceRow));
    resourceLayout->addWidget(m_cpuCount);
    resourceLayout->addSpacing(18);
    resourceLayout->addWidget(new QLabel(QStringLiteral("内存"), resourceRow));
    resourceLayout->addWidget(m_memoryMiB);
    resourceLayout->addStretch();
    form->addRow(QStringLiteral("资源"), resourceRow);

    m_vulkanPreset = new QCheckBox(
        QStringLiteral("Vulkan gfxstream 预设（WHPX + virtio-gpu-rutabaga-pci + surfaceless）"),
        m_configGroup);
    m_vulkanPreset->setChecked(true);
    form->addRow(QStringLiteral("后端"), m_vulkanPreset);

    m_extraArguments = new QPlainTextEdit(m_configGroup);
    m_extraArguments->setPlaceholderText(
        QStringLiteral("额外参数：每行一个，例如\n-net\nuser,id=net0"));
    m_extraArguments->setMaximumHeight(75);
    form->addRow(QStringLiteral("额外参数"), m_extraArguments);

    rootLayout->addWidget(m_configGroup);

    auto *hint = new QLabel(
        QStringLiteral("提示：启动器只证明 QEMU 参数和后端状态。guest Vulkan 操作及按 QEMU PID 的 Windows GPU Engine 采样仍需单独完成。"),
        central);
    hint->setWordWrap(true);
    rootLayout->addWidget(hint);

    auto *actionRow = new QHBoxLayout();
    m_startButton = new QPushButton(QStringLiteral("准备并启动 QEMU"), central);
    m_stopButton = new QPushButton(QStringLiteral("停止 QEMU"), central);
    auto *clearButton = new QPushButton(QStringLiteral("清空日志"), central);
    m_stopButton->setEnabled(false);
    actionRow->addWidget(m_startButton);
    actionRow->addWidget(m_stopButton);
    actionRow->addWidget(clearButton);
    actionRow->addStretch();
    rootLayout->addLayout(actionRow);

    m_status = new QLabel(QStringLiteral("状态：待配置"), central);
    m_backendStatus = new QLabel(QStringLiteral("Vulkan 后端：待验证"), central);
    rootLayout->addWidget(m_status);
    rootLayout->addWidget(m_backendStatus);

    m_commandSummary = new QPlainTextEdit(central);
    m_commandSummary->setReadOnly(true);
    m_commandSummary->setMaximumHeight(70);
    m_commandSummary->setPlaceholderText(QStringLiteral("启动命令摘要将在这里显示"));
    rootLayout->addWidget(m_commandSummary);

    m_log = new QPlainTextEdit(central);
    m_log->setReadOnly(true);
    m_log->setPlaceholderText(QStringLiteral("QEMU stdout/stderr 日志"));
    rootLayout->addWidget(m_log, 1);

    setCentralWidget(central);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startQemu);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopQemu);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(m_baseImage, &QLineEdit::textChanged, this, [this](const QString &value) {
        if (m_overlayImage->text().isEmpty() || m_overlayImage->text() == m_lastAutoOverlay) {
            m_lastAutoOverlay = defaultOverlayForBase(value);
            m_overlayImage->setText(m_lastAutoOverlay);
        }
    });
    connect(&m_controller, &QemuProcessController::logReceived,
            this, &MainWindow::appendLog);
    connect(&m_controller, &QemuProcessController::processStarted,
            this, &MainWindow::handleProcessStarted);
    connect(&m_controller, &QemuProcessController::processFinished,
            this, &MainWindow::handleProcessFinished);
    connect(&m_controller, &QemuProcessController::processError,
            this, &MainWindow::handleProcessError);
    connect(&m_controller, &QemuProcessController::backendEvidenceChanged,
            this, &MainWindow::handleBackendEvidence);
}

void MainWindow::browseQemuBinary()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择 QEMU system binary"),
                                                      {}, QStringLiteral("Executable (*.exe);;All files (*)"));
    if (!path.isEmpty()) {
        m_qemuBinary->setText(path);
    }
}

void MainWindow::browseQemuImgBinary()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择 qemu-img"),
                                                      {}, QStringLiteral("Executable (*.exe);;All files (*)"));
    if (!path.isEmpty()) {
        m_qemuImgBinary->setText(path);
    }
}

void MainWindow::browseBaseImage()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择 guest 基础镜像"),
                                                      {}, QStringLiteral("Disk images (*.qcow2 *.img *.raw);;All files (*)"));
    if (!path.isEmpty()) {
        m_baseImage->setText(path);
    }
}

void MainWindow::browseOverlayImage()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("选择 disposable overlay"),
                                                      m_overlayImage->text(),
                                                      QStringLiteral("QCOW2 images (*.qcow2);;All files (*)"));
    if (!path.isEmpty()) {
        m_overlayImage->setText(path);
        m_lastAutoOverlay.clear();
    }
}

LauncherConfig MainWindow::currentConfig() const
{
    LauncherConfig config;
    config.qemuBinary = m_qemuBinary->text().trimmed();
    config.qemuImgBinary = m_qemuImgBinary->text().trimmed();
    config.baseImage = m_baseImage->text().trimmed();
    config.overlayImage = m_overlayImage->text().trimmed();
    config.cpuCount = m_cpuCount->value();
    config.memoryMiB = m_memoryMiB->value();
    config.vulkanPreset = m_vulkanPreset->isChecked();
    config.extraArguments = parseExtraArguments(m_extraArguments->toPlainText());
    return config;
}

void MainWindow::startQemu()
{
    if (m_controller.isRunning()) {
        return;
    }

    const LauncherConfig config = currentConfig();
    const ValidationResult validation = validateConfig(config);
    if (!validation.ok) {
        setStatus(QStringLiteral("状态：配置校验失败"));
        QMessageBox::warning(this, QStringLiteral("配置校验失败"), validation.summary());
        return;
    }

    bool cancelled = false;
    const ExistingOverlayPolicy policy = chooseOverlayPolicy(&cancelled);
    if (cancelled) {
        return;
    }

    const OverlayResult overlay = ensureOverlay(config, policy);
    appendLog(overlay.message + QLatin1Char('\n'));
    if (!overlay.output.isEmpty()) {
        appendLog(overlay.output + QLatin1Char('\n'));
    }
    if (!overlay.ok) {
        setStatus(QStringLiteral("状态：overlay 准备失败"));
        QMessageBox::critical(this, QStringLiteral("overlay 准备失败"), overlay.message);
        return;
    }

    const QStringList arguments = buildQemuArguments(config);
    m_commandSummary->setPlainText(commandSummary(config.qemuBinary, arguments));
    setStatus(QStringLiteral("状态：参数已就绪，正在启动 QEMU"));
    m_backendStatus->setText(QStringLiteral("Vulkan 后端：等待 gfxstream 初始化（GPU 执行待验证）"));
    setRunningUi(true);
    m_controller.start(config.qemuBinary, arguments);
}

void MainWindow::stopQemu()
{
    m_controller.stop();
}

void MainWindow::clearLog()
{
    m_log->clear();
}

void MainWindow::appendLog(const QString &text)
{
    m_log->moveCursor(QTextCursor::End);
    m_log->insertPlainText(text);
    m_log->moveCursor(QTextCursor::End);
}

void MainWindow::handleProcessStarted()
{
    setStatus(QStringLiteral("状态：QEMU 运行中（Vulkan 执行证据待验证）"));
    appendLog(QStringLiteral("QEMU 进程已启动。\n"));
}

void MainWindow::handleProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    setRunningUi(false);
    if (status == QProcess::NormalExit && exitCode == 0) {
        setStatus(QStringLiteral("状态：QEMU 已正常停止"));
    } else {
        setStatus(QStringLiteral("状态：QEMU 失败（退出码 %1）").arg(exitCode));
    }
    appendLog(QStringLiteral("QEMU 已退出，exitCode=%1。\n").arg(exitCode));
}

void MainWindow::handleProcessError(const QString &message)
{
    appendLog(QStringLiteral("QEMU 进程错误：") + message + QLatin1Char('\n'));
    setStatus(QStringLiteral("状态：QEMU 启动失败"));
    if (!m_controller.isRunning()) {
        setRunningUi(false);
    }
}

void MainWindow::handleBackendEvidence(bool ready)
{
    if (ready) {
        m_backendStatus->setText(QStringLiteral("Vulkan 后端：gfxstream 已初始化（仍待 guest/GPU Engine 验证）"));
        appendLog(QStringLiteral("检测到 gfxstream 初始化和 Vulkan device/instance 日志；宿主 GPU 执行仍待验证。\n"));
    }
}

void MainWindow::setRunningUi(bool running)
{
    m_configGroup->setEnabled(!running);
    m_startButton->setEnabled(!running);
    m_stopButton->setEnabled(running);
}

void MainWindow::setStatus(const QString &text)
{
    m_status->setText(text);
}

ExistingOverlayPolicy MainWindow::chooseOverlayPolicy(bool *cancelled)
{
    *cancelled = false;
    if (!QFileInfo::exists(m_overlayImage->text())) {
        return ExistingOverlayPolicy::Reuse;
    }

    QMessageBox dialog(this);
    dialog.setWindowTitle(QStringLiteral("overlay 已存在"));
    dialog.setText(QStringLiteral("请选择如何处理已有 overlay：\n") + m_overlayImage->text());
    dialog.addButton(QStringLiteral("复用"), QMessageBox::AcceptRole);
    auto *recreate = dialog.addButton(QStringLiteral("重新创建"), QMessageBox::DestructiveRole);
    auto *cancel = dialog.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    dialog.exec();
    if (dialog.clickedButton() == cancel) {
        *cancelled = true;
        return ExistingOverlayPolicy::Reuse;
    }
    return dialog.clickedButton() == recreate ? ExistingOverlayPolicy::Recreate
                                               : ExistingOverlayPolicy::Reuse;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_controller.isRunning()) {
        event->accept();
        return;
    }

    const auto answer = QMessageBox::question(
        this, QStringLiteral("QEMU 仍在运行"),
        QStringLiteral("停止 QEMU 后关闭窗口吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::Yes) {
        m_controller.stop();
        if (!m_controller.isRunning()) {
            event->accept();
            return;
        }
    }
    event->ignore();
}

} // namespace launcher
