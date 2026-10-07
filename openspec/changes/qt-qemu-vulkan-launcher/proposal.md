# Proposal

## Why

当前仓库只有官方 QEMU submodule，Windows 上启动带 gfxstream Vulkan 的 QEMU 需要手工准备 QEMU 可执行文件、guest 磁盘 overlay、运行库路径和一组容易写错的参数。一个本地 Qt 图形界面可以把这些步骤集中起来，让实验人员用较少的配置启动目标虚拟机，同时保留日志和失败信息以便核验。

## What Changes

- 新增一个 Qt Widgets 桌面启动器，用于配置 QEMU 可执行文件、guest 基础镜像、overlay 镜像、CPU、内存和额外参数。
- 提供遵循 QEMU 项目本地 Vulkan 验证记录的 Vulkan 预设，自动带上 WHPX、`virtio-gpu-rutabaga-pci`、gfxstream Vulkan、surfaceless WSI 和 host memory 参数。
- 启动前检查关键路径和参数；需要时创建或刷新 disposable overlay，并通过 QEMU 子进程启动虚拟机。
- 提供启动、停止、清理和日志查看操作，区分参数校验失败、QEMU 进程失败和 Vulkan 证据尚未完成三类状态。
- 增加构建、运行和验证文档，明确界面启动成功不等于 guest Vulkan 已在宿主 GPU 上执行。

## Capabilities

### New Capabilities

- `qemu-vulkan-launcher`: 通过 Qt 界面配置、启动和停止带 Vulkan gfxstream 设备的 QEMU，并展示可核验的运行状态和日志。

### Modified Capabilities

- 无。

## Impact

- 在父仓库新增 Qt/C++ 桌面应用、构建配置、启动器文档和验证脚本；不修改 `third_party/qemu` submodule 内的 QEMU 源码。
- 依赖本机 Qt 6 Widgets、QEMU system binary 和 `qemu-img`；Windows Vulkan 实验还依赖构建时启用 rutabaga-gfx/gfxstream/Vulkan 的 QEMU 运行时。
- 应用通过 `QProcess` 直接传递参数，不经 shell 拼接；guest 磁盘默认使用 overlay 以避免改写基础镜像。
