# Tasks

## 1. Qt 应用与构建骨架

- [x] 1.1 创建 Qt 6 Widgets/CMake 工程、应用入口和基础窗口布局，并用 CMake 配置成功验证目标文件可生成
- [x] 1.2 建立配置模型和 Vulkan 预设的数据结构，加入 QEMU/qemu-img/基础镜像路径、CPU、内存和额外参数的校验，并用单元测试覆盖缺失文件与无效数值

## 2. 命令组装与 overlay

- [x] 2.1 实现 Vulkan 预设的独立参数组装，验证结果包含 WHPX、rutabaga gfxstream Vulkan、surfaceless 和 512M hostmem 核心参数且额外参数不会破坏预设
- [x] 2.2 实现通过 qemu-img 创建、复用和确认重建 qcow2 overlay 的流程，验证基础镜像不被写入并记录创建失败原因

## 3. QEMU 生命周期与界面反馈

- [x] 3.1 实现 QProcess 启动、停止、重复启动保护、退出码和 stdout/stderr 实时日志，使用一个可替换的测试进程验证正常退出与非零失败路径
- [x] 3.2 将表单、启动摘要、overlay 选择、日志区域和有限状态反馈接入主窗口，验证校验失败时进程不会启动且运行期间按钮状态正确
- [x] 3.3 处理关闭窗口时的运行中进程确认和停止流程，验证 QEMU 退出后窗口可以安全关闭

## 4. 文档与 Vulkan 验证

- [x] 4.1 编写 Windows 构建和运行文档，记录 Qt/QEMU 依赖、PATH 要求、guest overlay 用法和 Vulkan 预设参数，并验证文档命令可复现
- [x] 4.2 增加最小验证说明和日志字段，明确 gfxstream 初始化、guest Vulkan 操作和按 QEMU PID 的 GPU Engine 采样是三个独立证据，并通过一次 dry-run 检查状态不会误报 GPU 执行

## 5. 集成验证

- [x] 5.1 运行 OpenSpec 严格校验、Qt 构建和应用自检，确认父仓库工作区只包含本变更预期文件且 `third_party/qemu` gitlink 未被修改
