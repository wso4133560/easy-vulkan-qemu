# Qt QEMU Vulkan 启动器

这个工具是父仓库里的本地实验启动器，不修改 `third_party/qemu`。它用 Qt 6 Widgets 配置并直接启动 QEMU，默认使用 disposable qcow2 overlay，避免写入 guest 基础镜像。

## Windows 构建

需要 Qt 6 Widgets、Qt 6 Test、CMake、Ninja 和一套可运行的 QEMU。以 MSYS2 UCRT64 Qt 为例：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
ctest --test-dir build --output-on-failure
```

启动器位于 `build/qemu-vulkan-launcher.exe`。如果 Qt DLL 不在系统 PATH 中，可先运行：

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
```

## 使用方式

1. 选择带 `qemu-system-x86_64.exe` 和 `qemu-img.exe` 的 QEMU 运行目录。
2. 选择 guest 基础镜像；overlay 默认建议放在同目录下的 `*.overlay.qcow2`。
3. 保持 Vulkan gfxstream 预设勾选，检查 CPU 和内存，填写每行一个的额外参数（可选）。
4. 首次启动会调用 `qemu-img create -f qcow2 -F qcow2 -b <base> <overlay>`。已有 overlay 时，明确选择复用或重新创建。
5. 点击“准备并启动 QEMU”，观察命令摘要和 stdout/stderr 日志；运行中可点击“停止 QEMU”。

如果使用从源码构建、且 `pc-bios` 不在 QEMU 默认搜索路径中的版本，请在额外参数中逐行加入 `-L` 和 `D:\\code\\qemu\\pc-bios`。应用会把它们作为两个独立参数传递。

预设包含：

```text
-accel whpx
-m 2048
-smp 2
-drive file=<overlay>,if=virtio,format=qcow2
-display none
-device virtio-gpu-rutabaga-pci,gfxstream-vulkan=on,wsi=surfaceless,hostmem=512M
```

QEMU 必须是启用了 rutabaga-gfx/gfxstream/Vulkan 的构建。应用不会自动安装运行库、guest 镜像或 Vulkan 驱动。

## Vulkan 验证边界

界面按以下状态分别展示：

- 参数已就绪：输入校验通过，overlay 已复用或创建。
- QEMU 运行中：QEMU 子进程正在运行，Vulkan 执行证据仍待验证。
- gfxstream 已初始化：日志中同时看到 gfxstream 初始化和 Vulkan instance/device 相关信息；这仍不足以证明宿主 GPU 已执行工作。
- GPU 执行证据：需要 guest 侧 Vulkan 程序完成可重复的 GPU 操作，并在操作期间按 QEMU PID 采集 Windows `GPU Engine(*)\\Utilization Percentage`，保存时间戳、引擎实例和原始值。

guest 选择 gfxstream ICD 时，应显式设置 `VK_DRIVER_FILES`，避免被其他 ICD 隐藏。`wsi=surfaceless` 适合无窗口的 offscreen 验证，不代表桌面显示路径已验证。

## Dry-run 与故障定位

核心参数和 overlay 复用逻辑可在没有 QEMU 运行时的情况下验证：

```powershell
ctest --test-dir build --output-on-failure -R qemu-launcher-core-tests
```

测试会检查无效路径和资源值被拒绝、核心 Vulkan 参数不会被额外参数覆盖、overlay 创建参数使用 backing image，以及复用已有 overlay 不会删除文件。若 QEMU 进程启动失败，先看界面日志中的进程错误、退出码和 stderr；若只看到 guest 设备名称而没有 guest 操作结果与 GPU Engine 采样，状态应保持为“待验证”。

也可以运行一次完整 dry-run：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/launcher-dry-run.ps1 -BuildDir build
```

该脚本只验证启动器自身，不启动真实 QEMU；它会拒绝任何把未采集的 guest/GPU Engine 证据标记为“GPU 执行完成”的输出。
