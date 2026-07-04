# Celestia Runtime Model 输出分层说明

本文定位: 这是 Step20D-2 的输出分层说明，用来约束后续 Runtime Model、Scene 投影和跨进程消息格式的边界。本文不是最终协议规范，也不表示 Runtime 输出已经完成重构。

## 1. 当前源码事实

当前 runtime model 输出链路是:

```text
SimulationBackend::snapshot()
  -> ViewFrame
  -> ModelService::viewFrameResponse()
  -> view.frame

SimulationBackend::snapshot()
  -> ViewFrame
  -> extractSceneFrame(sessionId, ViewFrame)
  -> SceneFrame
  -> scene.frame
```

关键源码位置:

```text
src/celruntime/model/modelsnapshot.h
src/celruntime/model/modelservice.cpp
src/celruntime/model/realmodelbackend.cpp
src/celruntime/model/sceneextractor.h
src/celruntime/model/sceneextractor.cpp
src/celruntime/viewframe.h
src/celruntime/protocol/sceneprotocol.h
```

当前最大问题不是 `SceneFrame` 本身，而是 `ViewFrame` 同时承担了多种含义:

1. 作为 `SimulationBackend::snapshot()` 的返回值，它像 Model 后端快照。
2. 作为 `view.frame` payload，它是面向 View 的跨进程消息。
3. 作为 `extractSceneFrame()` 输入，它又是 Scene 投影的中间结构。
4. 在 `ModelService` 中，Controller/View 输入状态会直接写回 `ViewFrame`，再被序列化或投影。

因此，`ViewFrame` 当前应被理解为“历史形成的 runtime 投影结构”，不能被当作纯 ModelSnapshot。

## 2. 目标分层

后续应固定三层输出语义:

| 层 | 职责 | 不应包含 |
| --- | --- | --- |
| ModelSnapshot | Model 事实状态。包括时间、observer 事实状态、selection 语义、对象标识、轨道采样事实、资源事实引用。 | View3D 渲染状态、OpenGL/SDL/Renderer 对象、View 私有布局。 |
| SceneProjection | 面向 View 的场景投影。包括可见对象、相机候选、资源引用、标签候选、轨道线候选。 | 跨进程 envelope 细节、具体 View3D 绘制对象。 |
| SceneFrame | 跨进程消息 payload。稳定承载 SceneProjection 的序列化形式。 | 进程内指针、Renderer 句柄、Model 内部对象地址。 |

这三层不是命名游戏。后续判断一个字段放在哪一层时，应看它的产生原因:

```text
如果字段来自天体事实或仿真状态，优先属于 ModelSnapshot。
如果字段来自“给 View 看什么、如何组织成场景”，优先属于 SceneProjection。
如果字段只是为了跨进程序列化和版本兼容，属于 SceneFrame。
```

## 3. Step20D-2 不直接重构 ViewFrame 的原因

本轮不直接把 `ViewFrame` 拆成新的 `ModelSnapshot` 类型，原因是:

1. `ViewFrame` 已被 Step6、Step12、Step13、Step14、Step15、Step16、Step17 多组测试覆盖。
2. `ModelService` 当前同时支持 `view.frame` 和 `scene.frame` 两条消息路径。
3. `SceneExtractor` 的输入类型、View3D host、runtime session 路由和 golden fixtures 都依赖现有结构。
4. 在原 View3D 模板迁移前，过早冻结新的输出结构会再次形成“先造一个新 View 再倒逼 Model”的风险。

所以 Step20D-2 的正确成果是:

```text
明确当前 ViewFrame 不是纯 ModelSnapshot。
保留 runtime-model-projection 扫描债务作为后续阶段入口。
把 ModelSnapshot / SceneProjection / SceneFrame 的职责边界写清楚。
不在本轮进行大面积协议字段重命名或消息结构替换。
```

## 4. 当前扫描基线解释

Step20B-D 后，边界债务扫描结果为 64 项:

| 分类 | 数量 | 解释 |
| --- | ---: | --- |
| adapter->runtime | 1 | `SceneViewModel` 仍在 adapter 中包含 runtime `ViewFrame`。 |
| adapter->view | 12 | adapter 内仍有加载、投影、资源绑定等 View3D 依赖。 |
| model->adapter | 1 | `Universe` 仍包含 `adapter/solarsys.h`。 |
| runtime-model->app | 4 | `RealModelBackend` 仍直接使用应用层加载入口。 |
| runtime-model->view | 2 | `RealModelBackend` 仍使用 View3D mesh/texture manager 相关路径。 |
| runtime-model-projection | 44 | runtime model 仍直接读写 `ViewFrame` / `SceneFrame` / render state 投影结构。 |

其中 `runtime-model-projection` 不是 Step20D-2 要清零的目标。它是下一阶段拆分 `ModelSnapshot` 和 `SceneProjection` 的入口清单。

## 5. 与 Step20 代码治理的关系

Step20 已完成的 Model 侧实质治理:

1. `Simulation` 去掉无用 View3D texture include。
2. 生命周期事件从 `adapter` 移入 `model`。
3. `OrbitSampler` 不再依赖 `CurvePlot` / `CurvePlotSample`。
4. `ReferenceMark` 拆成 Model 侧 `BodyReferenceMark` 和 View3D 侧 `ReferenceMark`。
5. `Marker` / `MarkerRepresentation` 移入 Model，并去掉 Renderer 绘制方法。

这些工作使 `src/celengine/model` 当前不再直接包含 View3D/renderer 头，也不再命中 `model-view-symbol` 扫描分类。

## 6. 后续拆分门槛

只有满足以下条件时，才建议进入 runtime 输出结构重构:

1. 原 View3D 模板迁移已经明确需要哪些 ModelSnapshot 字段。
2. `ViewFrame` 中哪些字段是 Model 事实、哪些字段是 SceneProjection 已经逐项归类。
3. Step12-17 runtime 测试可以被分批迁移，而不是一次性改动所有消息路径。
4. 有新的 golden fixtures 覆盖 ModelSnapshot、SceneProjection、SceneFrame 三层转换。

## 7. Step20D-2 结论

Step20D-2 已完成“输出分层说明”和“剩余 runtime 投影债务归类”。本轮不应声称 Runtime Model 输出已经完全解耦；准确结论是:

```text
Model 核心源码边界比 Step20A 明显收敛。
Runtime Model 输出仍处于历史投影结构阶段。
ViewFrame 当前仍是过渡结构，不是最终 ModelSnapshot。
后续 View3D 模板迁移前，必须持续把 runtime-model-projection 作为显式剩余债务跟踪。
```
