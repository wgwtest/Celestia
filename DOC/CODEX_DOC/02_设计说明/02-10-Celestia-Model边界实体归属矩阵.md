# Celestia Model 边界实体归属矩阵

更新时间: 2026-07-03

本文定位: 这是 Step20A 的源码实体归属矩阵，用来回答“当前哪些源码实体属于 Model，哪些属于 Controller、加载构建、渲染资产绑定、Runtime 投影或 View3D 私有能力”。本文不是最终架构定案，也不表示 Model 层已经完成解耦；它是后续 Step20B-D 代码治理前的事实清单和执行入口。

## 1. 依据和范围

本文依据以下文档和源码事实:

| 类型 | 路径或命令 | 用途 |
| --- | --- | --- |
| 当前结构分析 | `DOC/CODEX_DOC/02_设计说明/02-09-Celestia当前源码结构与Model类族分析.md` | 提供对象定义、生命周期、调用者和 MVC 归属判断 |
| Step19.0 边界复审 | `DOC/CODEX_DOC/04_研制计划/37-WBS-0.37-Celestia标准MVC解耦-Step19.0模型控制视图边界复审.md` | 提供已确认边界污染点 |
| Step20 计划 | `DOC/CODEX_DOC/04_研制计划/38-WBS-0.38-Celestia标准MVC解耦-Step20Model层进一步解耦计划.md` | 提供阶段目标、工作包和验证口径 |
| 边界扫描 | `tools/mvc/scan_mvc_boundary_debt.ps1` | 提供当前 88 项边界债务基线 |
| 源码检索 | `rg` 对 `src/celengine`、`src/celruntime` 的检索结果 | 确认实体路径、include 和消费者 |

覆盖范围:

```text
src/celengine/model/
src/celengine/controller/
src/celengine/adapter/
src/celengine/view3d/
src/celrender/view3d/
src/celruntime/model/
src/celruntime/viewframe.h
src/celruntime/protocol/sceneprotocol.h
```

## 2. 分类词汇

本文使用以下归属分类:

| 分类 | 含义 |
| --- | --- |
| 纯 Model 对象图 | 表示 Celestia 天体对象、目录、对象关系和对象生命周期的核心数据 |
| Model 计算能力 | 时间阶段、参考系、轨道、自转、坐标转换等不依赖 View 的计算能力 |
| Controller 运行状态 | 当前时间推进、观察者、选择、导航、输入意图和运行会话状态 |
| 加载构建 | 从配置和数据文件构建 Model 对象图的过程和 builder |
| 渲染资产绑定 | 将 Model 对象关联到 mesh、texture、surface、geometry 等渲染资源 |
| View3D 私有实现 | 只服务原 View3D 绘制、OpenGL、渲染列表、可视表现和资源管理的能力 |
| Runtime 投影 | 从真实运行状态抽取供 runtime View 消费的投影数据 |
| 跨进程消息 | runtime 进程间传输的消息结构和序列化格式 |
| 混合/待拆分 | 当前同时承担多类职责，不能直接移动或锁定 |
| 暂缓判断 | 源码事实不足，或需要结合后续 View3D 读取链再判断 |

治理方式使用以下词汇:

| 治理方式 | 含义 |
| --- | --- |
| 保留 | 保持在当前层，后续只做接口或依赖清理 |
| 迁入 Model/Core | 移到 Model 或更中立的核心位置 |
| 拆分 | 将语义状态、计算能力、View 表现、资源绑定拆成不同实体 |
| 转入 View/Adapter | 从 Model 或 Controller 中移出 View 表现或渲染资源绑定 |
| 分层隔离 | 保留能力，但建立 ModelSnapshot、SceneProjection、SceneFrame 等边界 |
| 暂缓 | 暂不动源码，先补读取链分析或回归场景 |

## 3. 总体判断

当前 Model 层不是完全解耦状态。比较准确的判断是:

```text
原核心能力仍然保留。
真实 Model 后端已经能运行。
CMake 目标已经初步分层。
但源码依赖、Adapter 职责、Runtime 输出和 View3D 读取链仍然混杂。
```

当前不能宣布:

```text
Model 层已经完全锁死。
Model 层已经可以完全不动。
View3D 模板迁移可以直接开始。
SceneFrame 已经覆盖原 View3D 所需全部能力。
```

Step20A 的目标是把实体归属和扫描基线固定下来，为 Step20B-D 的代码治理提供入口。

## 4. 核心 Model 对象图

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `Universe` | `src/celengine/model/universe.h`, `src/celengine/model/universe.cpp` | Model 对象图聚合入口，持有 star/dso/solar system 目录，提供查找、标记、URL 查询等 | `Simulation`、原 View3D、UI、脚本、Runtime Model 后端 | 纯 Model 对象图 + 混合点 | 保留核心对象图；拆分 `Selection` 接口依赖和 `MarkerRepresentation` 表现依赖 | 边界扫描 `model->adapter`、`model-view-symbol`；对象查找回归 |
| `StarDatabase` / `Star` | `src/celengine/model/stardb.*`, `src/celengine/model/star.*` | 恒星目录、恒星对象、星表索引和 StarDetails | `Universe`、View3D、builder、Runtime 投影 | 纯 Model 对象图 | 保留；治理 `StarDetailsLifecycleEvents` 到 adapter 的反向依赖 | `model->adapter`；星表加载和 Sol 选择回归 |
| `DSODatabase` / `DeepSkyObject` | `src/celengine/model/dsodb.*`, `src/celengine/model/deepskyobj.*` | 深空对象目录、深空对象基类、空间索引 | `Universe`、View3D、builder、picker | 纯 Model 对象图 | 保留；治理 Nebula 生命周期事件和 render asset 绑定 | `model->adapter`；DSO 渲染/查询回归 |
| `SolarSystemCatalog` / `SolarSystem` | `src/celengine/adapter/solarsys.h`, `src/celengine/adapter/solarsys.cpp` | 按恒星索引组织太阳系，持有顶层 `PlanetarySystem` 和 `FrameTree` | `Universe`、`Simulation`、View3D、builder | Model 对象图，但源码位于 adapter | 不可直接按目录归类；应从 adapter 中拆出 model loading / catalog 方向 | `model->adapter` 中 `universe.h -> solarsys.h`；太阳系加载回归 |
| `PlanetarySystem` / `Body` | `src/celengine/model/body.*` | 行星系统和太阳系内天体对象，持有子系统、Timeline、FrameTree、物理属性和分类 | `SolarSystemsBuilder`、`Universe`、`Simulation`、View3D、UI | 纯 Model 对象图 + 混合点 | 保留核心 `Body`；拆分 `BodyFeaturesManager` 中显示属性和 `ReferenceMark` | `model->view`、`model-view-symbol`、Quick 回归 |
| `Location` | `src/celengine/model/location.*` | 依附于 `Body` 的地表地点对象 | `BodyFeaturesManager`、Universe 查找、UI、View3D | Model 信息对象 + View 消费 | 保留为 Model 信息对象；显示投影放入 Adapter/View | location 查询、地表标注回归 |

## 5. Model 计算能力

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `Timeline` | `src/celengine/model/timeline.*` | 管理某个 `Body` 的时间阶段集合，提供 `findPhase(t)` 等查询 | `Body`、`Selection`、`ObserverFrame`、View3D | Model 计算能力 | 保留；不得误认为全局仿真时间管理器 | 时间推进、跟随、goto 回归 |
| `TimelinePhase` | `src/celengine/model/timelinephase.*` | 描述某时间段内父对象、轨道、参考系、自转和 frame tree 关系 | `Timeline`、`FrameTree`、`Body`、View3D | Model 计算能力 | 保留；后续只处理 View 消费边界 | body 位置/姿态相关测试 |
| `ReferenceFrame` | `src/celengine/model/frame.*` | 给定时间计算姿态、角速度、惯性性质的参考系抽象 | `TimelinePhase`、`Body`、`ObserverFrame`、View3D | Model 计算能力 | 保留；Controller 可引用但不拥有 | reference frame 查询、observer frame 行为 |
| `FrameTree` | `src/celengine/model/frametree.*` | 管理参考系层级和阶段关系 | `SolarSystem`、`Body`、`TimelinePhase` | Model 计算能力 | 保留；归属不受 View3D 使用影响 | frame tree 相关编译和运行回归 |
| `OrbitSampler` | `src/celengine/model/orbitsampler.h` | 轨道采样，但当前直接使用 `CurvePlot` / `CurvePlotSample` | View3D 轨道绘制、Runtime orbit sample 相关路径 | Model 计算能力 + View 曲线耦合 | 拆分为纯采样点和 View3D `CurvePlot` 适配 | `model->view`、`model-view-symbol`；轨道显示和 runtime orbit sample |

## 6. Controller 运行状态

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `Simulation` | `src/celengine/controller/simulation.*` | 运行会话根对象，持有 `Universe`，管理时间推进、观察者、当前选择、暂停和导航 | `CelestiaCore`、原 View3D、Runtime Model 后端、Controller host | Controller 运行状态 + 高风险混合点 | 保留运行会话职责；清理 `view3d/texture.h` 残留；复审 ModelSession/ControllerSession 边界 | `controller->view`、Step16 交互回归 |
| `Observer` | `src/celengine/controller/observer.*` | 保存视点位置、姿态、速度、FOV、旅行动作、跟踪对象等 | `Simulation`、View3D、Runtime 投影 | Controller 运行状态 | 保留；不并入 `Universe`；输出进入 ModelSnapshot/SceneProjection 时分层 | time、camera、follow、goto、center 回归 |
| `ObserverFrame` | `src/celengine/controller/observer.*` | Controller 侧对 `ReferenceFrame` 的受限包装，保存坐标系统、参考对象、目标对象 | `Observer`、`Simulation`、View3D | Controller 运行状态，引用 Model 计算对象 | 保留包装角色；不得替代 `ReferenceFrame` | observer frame 行为和选择语义 |
| `Selection` | `src/celengine/controller/selection.*` | typed handle，对 Star/Body/DSO/Location 等指针做类型化包装 | `Simulation`、`Universe`、`ObserverFrame`、picker、UI、View3D | Controller 引用包装 + Model 接口污染点 | 保留轻量引用语义；复审 `Universe` 对 Controller 类型的接口依赖 | selection/clear/select Sol 回归 |

## 7. Model 与 View3D 交界实体

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `BodyFeaturesManager` | `src/celengine/model/body.*` | 以 `Body*` 为 key 保存 atmosphere、rings、locations、reference marks、orbit color、comet tail color 等 | `SolarSystemsBuilder`、`Universe`、View3D、UI | 混合/待拆分 | 逐字段分类；location/atmosphere/rings 与 reference mark/color 不能整体处理 | `model-view-symbol`、相关场景回归 |
| `ReferenceMark` | `src/celengine/view3d/referencemark.*`, `src/celengine/model/body.*` 中持有 | 参考标记表现，同时参与 `Body::recomputeCullingRadius()` | `BodyFeaturesManager`、View3D | Model 语义 + View3D 表现混合 | 拆分模型侧标记状态、边界计算数据和 View3D 渲染表现 | `model->view`、标记/边界/可见性回归 |
| `MarkerRepresentation` / `MarkerList` | `src/celengine/legacy/marker.*`, `src/celengine/model/universe.*` | 对象标记表现参数和列表 | `Universe`、UI、View3D | 标记语义 + View 表现混合 | `Universe` 保留标记语义，表现样式转入 View 状态或 Adapter | `model-view-symbol`、脚本标记回归 |
| `BodyRenderAssets` | `src/celengine/adapter/bodyrenderassets.*` | 将 `Body` / `RingSystem` 关联到 geometry、surface、alternate surface、ring texture 等 | View3D、builder、lifecycle | 渲染资产绑定 | 保持 View/Render Asset Binding，不能被 Model 反向依赖 | render asset 稳定性和 ResourceRef 回归 |
| `StarRenderAssets` | `src/celengine/adapter/starrenderassets.*` | 将 StarDetails 关联到 texture/geometry 等渲染资产 | View3D、stardb builder、lifecycle | 渲染资产绑定 | 与 StarDetails 生命周期事件解耦，保持订阅者角色 | star resource stability 回归 |
| `NebulaRenderAssets` | `src/celengine/adapter/nebularenderassets.*` | 将 Nebula 关联到 geometry | View3D、DSO builder、lifecycle | 渲染资产绑定 | 与 Nebula 生命周期事件解耦，保持订阅者角色 | DSO/nebula 资源回归 |

## 8. Adapter 目录实体归属

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `SolarSystemsBuilder` / `loadSSO` 相关 | `src/celengine/adapter/solarsys.*` | 解析太阳系数据，构建 `SolarSystem` / `Body`，同时写入 render assets | `CelestiaCore`、`RealModelBackend`、Universe 加载链 | 加载构建 + 渲染资产绑定混合 | 拆为 model loading 和 render asset binding 两段 | 太阳系加载、Quick 回归 |
| `StarDatabaseBuilder` | `src/celengine/adapter/stardbbuilder.*` | 解析星表，构建 `StarDatabase`，同时设置 star render assets | `loadStars`、`RealModelBackend` | 加载构建 + 渲染资产绑定混合 | 拆出纯星表构建，渲染资产绑定独立订阅或后处理 | star catalog 加载回归 |
| `DSODatabaseBuilder` | `src/celengine/adapter/dsodbbuilder.*` | 解析 DSO，构建 `DSODatabase`，同时加载 nebula render asset | `loadDSO`、`RealModelBackend` | 加载构建 + 渲染资产绑定混合 | 拆出 DSO 构建和 render asset 绑定 | DSO 加载回归 |
| `BodyLifecycleEvents` | `src/celengine/adapter/bodylifecycle.*` | Body 销毁、默认属性重置、ring 移除、shape override 查询事件 | `Body` 发布，`BodyRenderAssets` 订阅 | 应为 Model/Core 中立生命周期事件，当前位置不合适 | 迁入 Model/Core 或中立事件机制 | `model->adapter`、资源缓存回归 |
| `StarDetailsLifecycleEvents` | `src/celengine/adapter/stardetailslifecycle.*` | StarDetails 销毁、clone、copy、默认纹理等事件 | `StarDetails` 发布，`StarRenderAssets` 订阅 | 应为 Model/Core 中立生命周期事件，当前位置不合适 | 迁入 Model/Core 或中立事件机制 | `model->adapter`、star render assets 回归 |
| `NebulaLifecycleEvents` | `src/celengine/adapter/nebulalifecycle.*` | Nebula 销毁事件 | `Nebula` 发布，`NebulaRenderAssets` 订阅 | 应为 Model/Core 中立生命周期事件，当前位置不合适 | 迁入 Model/Core 或中立事件机制 | `model->adapter`、DSO 资源回归 |
| `SelectionPicker` | `src/celengine/adapter/selectionpicker.*` | 从输入射线和 render flags 中拾取对象 | View3D、Controller/UI、selection geometry provider | Controller/View 交界 | 不进纯 Model；后续归入 View input 或 Controller adapter | selection/picking 回归 |
| `BodyLocationGeometryProjector` | `src/celengine/adapter/bodylocationgeometryprojector.*` | 将地表位置转换为可投影/可拾取几何 | View3D、BodyFeaturesManager | 几何投影 / View adapter | 保持投影职责；不要反向进入 Model | location projection 回归 |
| `SceneViewModel` | `src/celengine/adapter/sceneviewmodel.*` | 从 `Simulation` 生成 runtime `ViewFrame` 快照 | `RealModelBackend`、runtime model | Runtime 投影 | 迁出普通 adapter，归入 runtime projection 或独立投影层 | `adapter->runtime`、runtime scene tests |
| `DeepSkyObjectRenderPolicy` | `src/celengine/adapter/deepskyobjectrenderpolicy.*` | DSO 类型到 render flags / labels 的映射 | View3D、picker | View 渲染策略 | 归入 View/Render policy，不进入 Model | DSO 渲染策略回归 |

## 9. Runtime Model 和输出分层

| 实体 | 源码路径 | 当前职责 | 当前消费者 | 初步归属 | 治理方式 | 验证入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `SimulationBackend` | `src/celruntime/model/modelsnapshot.h` | runtime Model 后端接口，当前 `snapshot()` 返回 `ViewFrame` | `ModelService`、Synthetic/Real backend | Runtime Model 服务接口 + 投影混合 | 拆分后应让 ModelSnapshot 与 SceneProjection 分层 | Step12-17 runtime tests |
| `RealModelBackend` | `src/celruntime/model/realmodelbackend.*` | 使用真实加载链创建 `Universe` / `Simulation`，输出 `ViewFrame` | `ModelService`、runtime assembly | 真实 Model 后端 + 加载依赖 + 投影混合 | 保留真实加载；隔离应用层加载函数、View3D 路径类和 projection 输出 | `runtime-model->app`、`runtime-model->view` |
| `ModelService` | `src/celruntime/model/modelservice.*` | Model host 服务，处理生命周期、命令、暂停、时间缩放、相机、选择、SceneFrame 请求 | Controller host、RuntimeSession | Runtime Model 服务 + Controller 命令处理混合 | 短期保留；后续拆清 Model command 与 Controller intent | Step16 交互闭环测试 |
| `SceneExtractor` | `src/celruntime/model/sceneextractor.*` | 将 `ViewFrame` 转换成 `SceneFrame`，带 fallback body/resource/render states | ModelService、View host | Runtime 投影 | 明确定义为 SceneProjection 到 SceneFrame 的投影，不是 Model 本体 | Step14/SceneFrame tests |
| `ViewFrame` | `src/celruntime/viewframe.h` | 当前 runtime 内部快照，含 camera、observer、resources、bodies、stars、orbits、selections | Model backend、SceneExtractor、codec | Runtime 投影快照，命名与职责需复审 | 分层为 ModelSnapshot / SceneProjection 后再决定命名 | `runtime-model-projection` |
| `SceneFrame` | `src/celruntime/protocol/sceneprotocol.h` | 跨进程 `scene.frame` 消息，供 View 消费 | View host、View3D host、runtime protocol | 跨进程消息 | 保持消息职责；不要宣称完整 Model 状态 | SceneFrame codec/validation tests |

## 10. 原 View3D 消费边界

原 View3D 当前不是平权模板 View。它直接或间接读取以下对象:

| 消费对象 | 读取方式 | 当前风险 | Step20 处理 |
| --- | --- | --- | --- |
| `Simulation` | View3D 读取时间、观察者、选择、Universe | View 直接依赖 Controller 运行对象 | 后续通过输入状态和投影数据替代直接读取 |
| `Universe` | View3D 读取对象目录、标记、查找结果 | View 直接依赖 Model 聚合入口 | 原 View3D 读取链完成前不得冻结输出 |
| `Observer` / `ObserverFrame` | View3D 读取相机位置、姿态、参考系 | Controller 状态直接暴露给 View | 拆成 ModelSnapshot / SceneProjection 的 observer/camera 字段 |
| `Body` / `Star` / `DSO` | View3D 读取对象属性、分类、位置、可见性 | View 直接消费对象图 | 需要逐项映射到投影层 |
| `BodyFeaturesManager` | View3D 读取 atmosphere、rings、locations、reference marks、颜色 | 显示属性和模型属性混在同一 manager | Step20D 前至少给出字段级治理结论 |
| RenderAssets / Texture/Mesh managers | View3D 读取渲染资源绑定 | 资源绑定路径和 Model 加载路径混在一起 | 归入渲染资产绑定和 data-plane 路径 |

结论: 原 View3D 迁移前，必须先完成 Model/Controller/Adapter 的交界实体判断。否则 `view3d_legacy` 会继承旧特殊位置。

## 11. 当前扫描基线

2026-07-03 本轮执行边界扫描，得到以下基线:

```text
MVC dependency scan passed
MVC CMake target scan passed
MVC boundary debt scan report: 88 finding(s)
MVC boundary debt scan self-test passed
```

边界债务分类:

| 分类 | 数量 | 代表问题 |
| --- | ---: | --- |
| `adapter->runtime` | 1 | `sceneviewmodel.h` include `celruntime/viewframe.h` |
| `adapter->view` | 12 | adapter 多处 include `view3d/meshmanager.h`、`renderflags.h`、`geometry.h` |
| `controller->view` | 1 | `simulation.h` include `view3d/texture.h` |
| `model->adapter` | 4 | `body.cpp`、`star.cpp`、`nebula.cpp`、`universe.h` 依赖 adapter |
| `model->view` | 2 | `body.cpp` include `referencemark.h`，`orbitsampler.h` include `curveplot.h` |
| `model-view-symbol` | 18 | `ReferenceMark`、`CurvePlot`、`MarkerRepresentation`、`MarkerList` |
| `runtime-model->app` | 4 | `realmodelbackend.cpp` include `celestia/configfile.h`、`load*.h` |
| `runtime-model->view` | 2 | `realmodelbackend.cpp` include `meshmanager.h`、`texmanager.h` |
| `runtime-model-projection` | 44 | `ViewFrame`、`SceneFrame`、`BodyRenderState`、`OrbitRenderState`、`LabelRenderState` 等投影状态 |

解释:

1. 88 项是当前 Step20A 基线，不是本轮新增退化。
2. 当前报告模式返回 0，用于可运行中间态审计。
3. 不能把 88 项全部清零作为唯一验收标准；每类债务必须先有归属判断、治理方式和回归入口。
4. 后续只有完成对应治理后，才能把相关分类切换为强制失败门槛。

## 12. 治理优先级

| 优先级 | 对象 | 原因 | 建议阶段 |
| --- | --- | --- | --- |
| P0 | `Simulation` 的 `view3d/texture.h` include | 低风险、可快速编译验证 | Step20B |
| P0 | 生命周期事件位置 | Model 反向依赖 Adapter，影响多处对象生命周期 | Step20C |
| P0 | `OrbitSampler` / `CurvePlot` | Model 数学能力直接依赖 View3D 曲线结构 | Step20C |
| P1 | `ReferenceMark` / `BodyFeaturesManager` | Model 与 View3D 混合最明显，但影响边界/标记/可见性 | Step20D |
| P1 | `MarkerRepresentation` / `Universe` | 标记语义和 View 表现混合，影响脚本/UI | Step20D |
| P1 | `SceneViewModel` / `ViewFrame` / `SceneFrame` | Runtime 输出分层不清，影响后续 View 输入规范 | Step20D |
| P2 | builder 中的数据加载和 render asset 绑定 | 影响加载链和资源稳定性，需在前序治理后处理 | Step20D 或后续 |
| P2 | Picker / Projector / RenderPolicy | 与原 View3D 读取链强相关，需结合模板 View 迁移判断 | Step20D 或 View3D 阶段 |

## 13. Step20A 结论

Step20A 当前可得出的结论:

```text
Model 边界实体归属矩阵已经建立第一版。
边界债务扫描基线已经固定为 88 项。
Step20B 可以优先处理低风险 include 清理，但不能直接开始中高风险拆分。
View3D 模板迁移仍不具备直接执行条件。
```

Step20A 不能得出的结论:

```text
Model 层已经解耦完成。
Model 层后续完全不用改。
扫描债务清零就是验收完成。
原 View3D 迁移只剩目录整理问题。
```

## 14. 下一步建议

建议下一步执行 Step20B，范围严格限制为低风险边界清理:

1. 处理 `src/celengine/controller/simulation.h` 中疑似未使用的 `#include <celengine/view3d/texture.h>`。
2. 删除前后分别运行边界扫描，确认 `controller->view` 分类从 1 变为 0。
3. 编译 `celestia_controller`、`celestia-sdl`、`unit`。
4. 执行 Quick 回归，确认统一 exe 原能力不退化。

只有 Step20B 通过后，才建议进入生命周期事件和 `OrbitSampler` 拆分。

