# Celestia 当前解耦现状与 MVC 能力盘点

日期：2026-06-27

## 1. 结论摘要

截至 `c6d7e53 test: add Celestia compatibility regression harness`，当前 `master` 同时保留三条能力线：

1. **普通统一 exe / in-process 主路径**：仍由 `celestia-sdl.exe` 创建 `CelestiaCore`、初始化 `Simulation`、创建 SDL/OpenGL 窗口并运行历史渲染管线。这是当前最接近原 Celestia 用户能力的主路径。
2. **源码与构建层 MVC 边界**：`src/celengine` 已物理拆为 `model` / `controller` / `adapter` / `view3d` / `legacy`，并由 `celestia_model`、`celestia_controller`、`celestia_view_adapter`、`celestia_view3d` object target 固化依赖方向。
3. **本机多进程 MVC runtime 路径**：`src/celruntime` 已具备协议、host 进程、transport、runtime assembly、View plugin、Debug2D View、OpenGL3D View host、View 切换和本机 smoke 验证。

当前不能下的结论是：跨进程 OpenGL3D View 已经具备完整历史 Renderer 视觉等价，或独立 Model 进程已经完整承载真实 Celestia Universe / StarDatabase / Body / Orbit 状态。

## 2. 证据索引

| 证据类别 | 主要文件 |
| --- | --- |
| 最终架构说明 | `DOC/CODEX_DOC/02_设计说明/02-07-Celestia标准MVC最终架构说明.md` |
| Runtime 协议 | `DOC/CODEX_DOC/02_设计说明/02-06-Celestia-MVC-Runtime-Protocol-v1.md` |
| Step12 scene.frame vNext 契约 | `DOC/CODEX_DOC/02_设计说明/02-09-Celestia真实场景投影与SceneFrame契约.md` |
| 构建边界 | `src/celengine/CMakeLists.txt`, `src/celruntime/CMakeLists.txt`, `src/celestia/CMakeLists.txt` |
| runtime 入口 | `src/celestia/sdl/sdlmain.cpp` |
| 边界扫描 | `tools/mvc/scan_mvc_dependencies.ps1`, `tools/mvc/scan_cmake_targets.ps1` |
| 原能力回归 | `tools/regression/run_celestia_compat_regression.ps1` |
| 当前 Full 机测 | `DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-06-27-155813-Celestia-compat-regression-machine-report.md` |

## 3. 当前总体架构状态

### 3.1 普通统一 exe 主路径

无 `--runtime-config`、无 `--mvc-mode=multi-process` 时，SDL 入口仍走历史应用路径：

```text
celestia-sdl.exe
  -> CelestiaCore::initLocale
  -> Environment::init
  -> CelestiaCore::initSimulation
  -> Environment::createAppWindow
  -> window->run
```

这条路径仍链接 `celestia` shared library，而该 library 汇入 Model、Controller、ViewAdapter、View3D、Runtime、脚本、图像、模型、字体等 object target。它不是纯粹的三进程架构，而是保留原 Celestia 可用性的兼容主路径。

### 3.2 源码与构建层 MVC

`src/celengine/CMakeLists.txt` 固化了当前源代码 ownership：

```text
src/celengine/model       -> celestia_model
src/celengine/controller  -> celestia_controller
src/celengine/adapter     -> celestia_view_adapter
src/celengine/view3d      -> celestia_view3d
src/celengine/legacy      -> celengine
```

依赖方向为：

```text
celestia_controller    -> celestia_model
celestia_view_adapter  -> celestia_model + celestia_controller
celestia_view3d        -> celestia_model + celestia_controller + celestia_view_adapter
celengine legacy       -> celestia_model
```

`celestia_model` 当前不反向链接 Controller / Adapter / View3D。边界扫描会拒绝 protocol 层包含 SDL、OpenGL、`celrender` 或 `celengine/view3d`，也会拒绝已移除的 Step6 file-script fallback 回到生产源码。

### 3.3 多进程 runtime

`src/celruntime` 当前包含：

```text
protocol      RuntimeEnvelope, lifecycle, view.input, scene.frame
model         ModelService, SimulationBackend, SceneExtractor
controller    ControllerService
view          Debug2D ViewService
view3d        OpenGL3D View host and SDL/OpenGL loop
process       RuntimeSession, ProcessSupervisor, host loops
transport     stdio-pipe, local-socket, framed transport
assembly      RuntimeAssemblyConfig, RuntimeAssemblyRunner
dataplane     DataPlaneRef, in-process and shared-memory channel
viewplugin    manifest, registry, lifecycle
```

当前 runtime 可启动以下本机 host：

```text
celestia-model-host
celestia-controller-host
celestia-view-host
celestia-view3d-host
```

支持的验证路径包括：

```text
runtime-2d-stdio.yaml
runtime-3d-stdio.yaml
runtime-2d-local-socket.yaml
runtime-3d-local-socket.yaml
runtime-switch-2d-to-3d-local-socket.yaml
runtime-switch-3d-to-2d-local-socket.yaml
```

## 4. Model 端能力

### 4.1 已具备的源码 Model 能力

`src/celengine/model` 已承载以下原 Celestia 语义：

| 能力 | 当前落点 |
| --- | --- |
| Universe 聚合与对象查找 | `model/universe.*` |
| 恒星、星表、星名、星类 | `model/star.*`, `model/stardb.*`, `model/starname.*`, `model/stellarclass.*` |
| 太阳系天体、表面、位置、时间线 | `model/body.*`, `model/surface.h`, `model/location.*`, `model/timeline.*`, `model/timelinephase.*` |
| 深空对象与目录 | `model/deepskyobj.*`, `model/dsodb.*`, `model/dsooctree.*`, `model/nebula.*`, `model/opencluster.*` |
| 坐标与参考系 | `model/frame.*`, `model/frametree.*`, `model/univcoord.h` |
| 星座、边界、asterism、category | `model/constellation.*`, `model/boundaries.*`, `model/asterism.*`, `model/category.*` |
| 轨道/旋转 manager 接口侧语义 | `model/trajmanager.*`, `model/rotationmanager.*`，底层天文计算仍在 `src/celephem` / `src/celastro` |

这部分是“进程内 Model 源码边界”的成果，不等于这些真实对象已经完整搬入独立 Model host。

### 4.2 已具备的 runtime Model 能力

`src/celruntime/model/ModelService` 当前具备：

| 能力 | 说明 |
| --- | --- |
| 长期 host service | 支持 `runtime.start` / `runtime.shutdown` lifecycle |
| 时间控制 | 支持 `model.setTime`, `model.setTimeScale`, `model.step`, `model.setPaused` |
| Snapshot 输出 | 支持 `model.requestSnapshot` 输出 `view.frame` |
| 3D scene 输出 | 支持 `model.requestSceneFrame` 或带 `view=celestia.view3d.opengl` 的 `model.step` 输出 `scene.frame` |
| scene.frame vNext 契约 | Step12 分支已定义 `protocolVersion`、`TimeState`、`ResourceRef` vNext、结构化 label、DSO 预留和 validation helper |
| View input 回写 | 支持 `model.setViewInput`，把输入摘要体现在后续 `scene.frame` structured label 中 |
| Backend 抽象 | `SimulationBackend` 提供 `load` / `setTime` / `step` / `snapshot` 接口 |

### 4.3 Model 端明确未完成

| 未完成项 | 说明 |
| --- | --- |
| 真实 Celestia Universe 进程化 | 默认 backend 是 `SyntheticSimulationBackend`，不是完整 `Universe` / `Simulation` / catalog 运行体 |
| 真实 catalog 进程内加载到独立 Model host | `RuntimeDataPaths` 已有 `dataRoot`，但 ModelService 默认实现没有加载完整 Celestia 数据目录 |
| 完整天体、星表、轨道、姿态输出 | `scene.frame` vNext 已具备契约字段，但当前仍只输出简化 body / star / orbit / label 状态和 placeholder resource |
| 高吞吐资源数据面默认化 | shared memory channel 已有最小测试通道，但并非所有 scene/resource payload 默认走共享内存 |

## 5. Controller 端能力

### 5.1 已具备的源码 Controller 能力

`src/celengine/controller` 当前包含：

| 能力 | 当前落点 |
| --- | --- |
| Simulation 状态推进 | `controller/simulation.*` |
| Selection union 语义 | `controller/selection.*` |
| Observer / ObserverFrame 状态 | `controller/observer.*` |

构建上 `celestia_controller` 只依赖 `celestia_model`。这表示 Controller 源码层已从渲染资源和窗口系统中收缩出来。

### 5.2 已具备的 runtime Controller 能力

`src/celruntime/controller/ControllerService` 当前具备：

| 能力 | 说明 |
| --- | --- |
| lifecycle | 支持 start / shutdown 并返回 running / stopped |
| tick 编排 | `controller.tick` 可转为 `model.requestSnapshot` 或 `model.step` |
| 3D tick | 当 payload 指定 `view=celestia.view3d.opengl` 时，转发为带 3D view 意图的 `model.step` |
| View input 转 Model command | `view.input` 可转为 `model.setViewInput` |
| 退出事件 | `Quit` 或 `input.closeRequested` 可广播 `runtime.shutdown` |
| 最小暂停控制 | `input.key` 中 Space 可转为 `model.setPaused` |

### 5.3 Runtime 编排能力

`ProcessSupervisor`、`RuntimeSession`、`RuntimeAssemblyRunner` 当前具备：

| 能力 | 当前状态 |
| --- | --- |
| 启动 model/controller/view host | 已实现 |
| ready / stopped / error lifecycle | 已实现 |
| stdio-pipe transport | 已实现 |
| local-socket transport | 已实现，本机 Windows named-pipe 语义 |
| Runtime config 文件装配 | 已实现 |
| 2D / 3D View 切换 | 已实现 ordered stop/start |
| trace file | 已实现基本 session/transport/view/exitCode 记录 |

### 5.4 Controller 端明确未完成

| 未完成项 | 说明 |
| --- | --- |
| 完整用户输入语义 | 当前只覆盖最小 key / close / generic view.input 映射，不是完整 Celestia 导航、选择、时间、菜单命令集 |
| 完整真实 SimulationBackend 控制 | 多进程 Controller 目前控制 synthetic Model backend，不是完整历史 `Simulation` 实例 |
| 跨机器部署 | local-socket 是本机 IPC，不是 TCP / 多机分布式部署 |
| 任意 View 热替换 | 当前保证 ordered stop/start，不保证渲染循环任意帧的无缝热替换 |

## 6. View 端能力

### 6.1 View Adapter

`src/celengine/adapter` 是当前 View Adapter 的主要落点：

| 能力 | 当前落点 |
| --- | --- |
| 拾取与几何 provider | `selectionpicker.*`, `selectiongeometryprovider.h`, `deepskyobjectpicker.*` |
| body/star/nebula render sidecar | `bodyrenderassets.*`, `starrenderassets.*`, `nebularenderassets.*` |
| 生命周期事件 | `bodylifecycle.*`, `stardetailslifecycle.*`, `nebulalifecycle.*` |
| DSO render policy | `deepskyobjectrenderpolicy.*` |
| geometry projection / resource loader | `bodylocationgeometryprojector.*`, `nebularenderassetloader.*` |
| renderer-free snapshot | `sceneviewmodel.*` |
| catalog build adapter | `solarsys.*`, `stardbbuilder.*`, `dsodbbuilder.*` |

这些模块让 Model / Controller 不直接暴露 Renderer、GeometryManager、TextureManager、RenderFlags 等 View 资源。

### 6.2 历史 in-process 3D View

`src/celengine/view3d` 与 `src/celrender/view3d` 仍保存历史 OpenGL 3D 渲染能力，包括 Renderer、TextureManager、GeometryManager、shader、framebuffer、overlay、网格和纹理相关模块。

普通 SDL exe 路径仍通过 `CelestiaCore` 和 SDL window 使用这条历史渲染管线。这是当前用户可见能力保真的核心。

### 6.3 Debug2D runtime View

`src/celruntime/view/ViewService` 当前可：

| 能力 | 说明 |
| --- | --- |
| lifecycle | start / shutdown |
| 消费 `view.frame` | 反序列化 ViewFrame，记录 frameId / time / summary / frameCount |
| 发起关闭事件 | `closeRequested()` 输出 `input.closeRequested` |

Debug2D 主要用于 runtime 闭环和协议验证，不是完整视觉 View。

### 6.4 OpenGL3D runtime View

`src/celruntime/view3d` 当前可：

| 能力 | 说明 |
| --- | --- |
| 独立 3D host | `celestia-view3d-host` 可作为独立 View host 运行 |
| 消费 `scene.frame` | 反序列化 SceneFrame，记录 sequence / time / bodyCount / starCount |
| SDL/OpenGL loop | `View3DLoop` 创建 SDL window、OpenGL context，执行简化绘制 |
| 输入回传 | SDL 事件可转为 `view.input` 返回 Controller |
| frameRendered 事件 | 渲染后回传 `view.frameRendered` |

当前 `View3DLoop` 绘制的是 scene.frame 驱动的简化星点、轨道和 body 占位图形。它证明跨进程 3D View 的进程、协议、窗口、OpenGL context 和输入回流可工作，但还不是历史 Renderer 的完整视觉等价实现。

### 6.5 View plugin 与 manifest

当前内置 View manifest：

```text
src/celviews/debug2d/celestia-view2d-debug.plugin.json
src/celviews/opengl3d/celestia-view3d-opengl.plugin.json
```

已定义的能力包括 `view.frame`、`scene.frame`、`view.input`、`debug2d`、`opengl`、`sdl-window`。当前更准确的说法是“内置 View provider / manifest registry 已具备”，而不是“第三方插件生态已稳定发布”。

## 7. 兼容层与遗留模块状态

| 项 | 当前判断 |
| --- | --- |
| `src/celengine/legacy` | 保留混合历史模块，如 galaxy / globular / marker / starbrowser；它们是历史产品兼容模块，不应简单理解为待删除 MVC shim |
| Step6 file-script fallback | 已从生产路径移除，扫描脚本会拒绝 `stdio-files` 等遗留 fallback |
| 根 forwarding header | Step11 后已作为扫描项处理，避免旧 public include 根路径继续掩盖 ownership |
| 普通统一 exe | 不是“未解耦残留垃圾”，而是当前用户能力保真的主路径和回归参照 |
| runtime 多进程路径 | 是新架构能力承载路径，但仍处于逐步接入真实 Celestia scene/resource 的阶段 |

## 8. 当前可以对外表述的能力

可以表述为：

```text
Celestia 当前 master 已具备源码/构建层 MVC 边界、本机多进程 runtime 骨架、Debug2D 与 OpenGL3D host 验证路径，并保留普通 SDL 统一 exe / in-process 历史能力路径。
```

可以表述为：

```text
M / C / V host 进程可以被 runtime assembly 启动、监管、通信、切换 View 并关闭。
```

不能表述为：

```text
跨进程 OpenGL3D View 已达到历史 Celestia Renderer 的完整视觉能力。
```

不能表述为：

```text
独立 Model 进程已经完整承载真实 Celestia Universe、StarDatabase、SolarSystem、Orbit、RotationModel 和全部 catalog 资源。
```

不能表述为：

```text
Qt / Win32 前端能力已经由当前 SDL 回归脚本覆盖。
```

## 9. 下一步建议

当前 Step12 的目标是 `scene.frame vNext` 契约和 Model Lock Policy。Step12 不等于真实 Model host 已完成，也不等于 View3D 已具备视觉等价。

下一阶段应命名为类似：

```text
Step12 - Real Celestia scene projection and View3D visual parity path
```

建议优先级：

1. 把真实 `Universe` / `Simulation` / `Observer` / `Selection` 的只读快照投影为 `scene.frame`。
2. 替换 `SyntheticSimulationBackend` 或为其增加真实 Celestia backend。
3. 扩展 `ResourceRef`，覆盖真实 catalog、mesh、texture、orbit、label、render setting。
4. 将历史 Renderer 能力逐步接入 `celestia-view3d-host`，同时保持进程边界。
5. 把 `view.input` 扩展到真实导航、选择、时间控制和相机命令。
6. 继续使用普通 in-process SDL Full 回归作为原能力不退化的参照。
