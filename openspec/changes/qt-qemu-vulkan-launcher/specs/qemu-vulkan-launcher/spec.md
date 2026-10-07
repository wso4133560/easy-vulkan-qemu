# Spec Delta

## Purpose

为 Windows 上的本地 Vulkan 虚拟机实验提供一个简单、可重复且可观察的 QEMU 启动入口，降低手工输入参数和误改 guest 基础镜像的风险，同时让用户清楚知道哪些结果已经验证、哪些结果仍需 guest 与宿主侧证据。

## ADDED Requirements

### Requirement: 配置并校验 QEMU 启动输入

启动器 SHALL 允许用户选择或填写 QEMU system binary、`qemu-img`、guest 基础镜像、disposable overlay 路径、CPU 数量、内存大小和额外 QEMU 参数。启动前 SHALL 校验必需文件存在、CPU 和内存为正值，并在校验失败时阻止启动且指出具体字段。

#### Scenario: 配置通过

- **WHEN** 用户选择存在的 QEMU system binary、`qemu-img` 和基础镜像，并填写有效 CPU 与内存
- **THEN** 启动操作可用，界面展示将使用的 overlay 路径和关键参数

#### Scenario: 配置缺失

- **WHEN** QEMU binary 或基础镜像不存在，或 CPU/内存不是正值
- **THEN** 启动器 SHALL 阻止 QEMU 进程启动，并在界面中标出修正项

### Requirement: 提供可复用的 Vulkan 启动预设

启动器 SHALL 提供一个 Vulkan gfxstream 预设，生成的 QEMU 参数 SHALL 包含 `-accel whpx`、`virtio-gpu-rutabaga-pci`、`gfxstream-vulkan=on`、`wsi=surfaceless` 和 `hostmem=512M`，并允许用户在不删除这些核心参数的前提下追加额外参数。界面 SHALL 明确该预设要求 QEMU 构建启用 rutabaga-gfx/gfxstream/Vulkan。

#### Scenario: 使用 Vulkan 预设启动

- **WHEN** 用户选择 Vulkan 预设并通过输入校验后点击启动
- **THEN** 启动器 SHALL 以独立参数启动 QEMU，并在可查看的命令摘要中显示上述 Vulkan 设备参数、CPU、内存和 guest overlay

#### Scenario: 追加 guest 参数

- **WHEN** 用户填写额外 QEMU 参数
- **THEN** 启动器 SHALL 将其作为独立参数追加，保留预设的 Vulkan 核心参数并避免 shell 字符串重新解析

### Requirement: 使用 disposable overlay 保护基础镜像

启动器 SHALL 在 overlay 不存在时调用匹配的 `qemu-img` 创建基于基础镜像的 qcow2 overlay，并在 overlay 已存在时提供复用或重新创建的明确选择。重新创建操作 SHALL 要求用户确认，并 SHALL 不修改基础镜像。

#### Scenario: 首次创建 overlay

- **WHEN** 用户选择不存在的 overlay 路径并确认启动
- **THEN** 启动器 SHALL 创建 overlay，记录创建结果，并将该 overlay 传给 QEMU

#### Scenario: 复用已有 overlay

- **WHEN** overlay 已存在且用户选择复用
- **THEN** 启动器 SHALL 保留已有 overlay 内容并用它启动 QEMU

### Requirement: 管理 QEMU 生命周期并暴露日志

启动器 SHALL 提供启动、停止和清理运行状态的操作；运行期间 SHALL 禁止重复启动；QEMU 标准输出和标准错误 SHALL 持续显示在日志区域，并保留退出码和最后一条错误摘要。关闭界面时 SHALL 提示仍在运行的 QEMU 进程，并提供停止后再关闭的路径。

#### Scenario: 正常启动与停止

- **WHEN** QEMU 成功启动后用户点击停止
- **THEN** 启动器 SHALL 请求 QEMU 退出，显示最终退出状态，并恢复启动控件

#### Scenario: QEMU 启动失败

- **WHEN** QEMU binary 无法启动或很快以非零退出码结束
- **THEN** 启动器 SHALL 显示进程错误、退出码和日志摘要，并允许用户修改配置后重试

### Requirement: 区分 Vulkan 启动状态与 GPU 执行证据

启动器 SHALL 将“参数已组装”“QEMU 进程运行”“日志出现 gfxstream 初始化”作为独立状态，并在界面和文档中说明：guest `vulkaninfo` 设备名本身不足以证明宿主 GPU 执行。应用 SHALL 提供指向 guest Vulkan 操作和 Windows `GPU Engine` 采样记录的验证提示，但不得将未采集的证据标记为已通过。

#### Scenario: 只有进程运行

- **WHEN** QEMU 正在运行但尚未出现 gfxstream 初始化日志或 guest/宿主采样结果
- **THEN** 界面 SHALL 显示 QEMU 正在运行，同时将 Vulkan GPU 执行标记为“待验证”

#### Scenario: 观察到初始化日志

- **WHEN** 日志出现 gfxstream 初始化和 Vulkan device 创建相关信息
- **THEN** 界面 SHALL 更新为“后端已初始化”，但仍要求 guest GPU 操作和按 PID 的 GPU Engine 采样才能标记执行证据完成
