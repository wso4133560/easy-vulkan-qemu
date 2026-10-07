# Design

## Context

父仓库当前只有官方 QEMU submodule，应用代码需要从零建立。`D:\code\qemu\AGENTS.md` 给出的 Windows 实验配置要求 WHPX、`virtio-gpu-rutabaga-pci`、gfxstream Vulkan、surfaceless WSI 和 512 MiB host memory，并要求使用基础磁盘的 disposable overlay。设计必须支持本地实验，不修改 QEMU 子模块，也不能把进程启动当作 GPU 执行证明。

## Goals / Non-Goals

**Goals:**

- 提供一个 Windows 优先的 Qt 6 Widgets 启动器，减少重复输入和参数错误。
- 用可编辑表单生成独立的 QEMU 参数，安全管理 QEMU 子进程和日志。
- 保留基础镜像，通过 `qemu-img` 管理 overlay，并为 Vulkan 验证显示清晰的状态边界。
- 让构建、运行和最小验证可以在仓库文档中复现。

**Non-Goals:**

- 不在本变更中修改或重新实现 QEMU、rutabaga-gfx、gfxstream 或 guest Mesa。
- 不自动安装 Qt、QEMU、guest 镜像或 Vulkan 驱动。
- 不把 guest `vulkaninfo`、单条日志或界面启动成功解释为宿主 GPU 利用率证明。
- 不实现完整 VM 编排、快照管理、远程控制或图形桌面显示配置。

## Decisions

### 直接使用 Qt Widgets 与 QProcess

应用使用 Qt 6 Widgets 和 `QProcess`，以 QStringList 逐项传递参数。这样可以在 Windows 上避免 shell 引号和转义差异，并能实时读取 stdout/stderr。相比脚本或拼接命令行，代价是需要本机 Qt 运行库；这是可接受的桌面工具依赖。

### 将配置、命令组装和进程控制分层

表单层只负责编辑值和展示状态；配置对象负责校验；命令组装器负责把 Vulkan 预设、overlay、CPU、内存和额外参数转换为参数列表；进程控制器负责启动、停止、退出码和日志。这样可在不启动真实 VM 的情况下测试校验与参数组装。

### Vulkan 预设采用显式核心参数

预设固定加入 `-accel whpx` 与 `virtio-gpu-rutabaga-pci,gfxstream-vulkan=on,wsi=surfaceless,hostmem=512M`，并允许追加参数。预设不替用户选择 guest 内核、rootfs 或窗口系统，因为 `wsi=surfaceless` 主要支持无窗口的验证工作负载。

### Overlay 通过 qemu-img 创建

启动器调用与 QEMU 配套的 `qemu-img` 创建 qcow2 backing overlay，并在已有 overlay 时显式询问复用或重建。创建动作单独记录到日志，基础镜像路径以只读输入处理，避免启动器误写源盘。

### 证据状态使用保守的有限状态

界面显示 `待配置`、`参数已就绪`、`QEMU 运行中`、`gfxstream 已初始化`、`待 guest/GPU Engine 验证` 和 `已停止/失败` 等状态。只有日志、guest 操作结果和按 QEMU PID 的 Windows GPU Engine 采样都齐全时，文档才允许报告宿主 GPU 执行完成。

## Risks / Trade-offs

- [QEMU 构建缺少 gfxstream/Vulkan] → 启动前显示构建要求，运行时保留完整 stderr，并把初始化失败归类为后端失败。
- [本机未安装或 PATH 未包含 Qt/QEMU 运行库] → 提供路径选择和构建/运行前检查；不在应用内静默修改系统环境。
- [已有 overlay 包含旧 guest 状态] → 启动前显示 overlay 路径和复用/重建选择，重建要求确认。
- [surfaceless 无法证明桌面显示] → 文档和状态提示使用无窗口 guest Vulkan 操作，明确不覆盖 X11/Wayland/Windows 桌面呈现。
- [GPU Engine 采样采集范围不一致] → 只提示按 QEMU PID 过滤的原始计数器记录，并要求记录时间戳、引擎实例和采样窗口。

## Migration Plan

1. 在父仓库添加 Qt 应用和构建文件，不触碰 `third_party/qemu` 的 gitlink。
2. 在 Windows 上用 Qt 6 配置并构建，先运行参数组装和界面测试，再使用 disposable overlay 启动 QEMU。
3. 若应用不再需要，可删除新增应用目录和 OpenSpec 变更；QEMU submodule 与基础镜像不受影响。

