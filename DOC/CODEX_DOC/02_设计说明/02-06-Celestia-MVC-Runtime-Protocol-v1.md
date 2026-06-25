# Celestia MVC Runtime Protocol v1

> 本文定义 Step6 后 M / C / V 独立进程之间共享的运行时协议。它是 transport-independent 的协议说明，不绑定 stdio、named pipe、local socket 或 TCP。当前代码只实现 framed stdio，后续 transport 必须复用本协议语义。

## 1. 状态

```text
Status: Implemented in Step6 smoke path
Date: 2026-06-25
Primary transport: framed stdio
Primary runtime mode: local multi-process smoke
Primary view path: Debug 2D / headless ViewService
3D OpenGL cross-process rendering: not implemented in Step6
```

## 2. 角色

| Role | 进程 / 模块 | 职责 |
| --- | --- | --- |
| `launcher` | `celestia-sdl.exe` / `ProcessSupervisor` | 启动 host、发送 lifecycle、收集 ready/stopped、执行本机 smoke 编排 |
| `model` | `celestia-model-host` | 持有模型状态，处理 `model.*` command，产出 `view.frame` |
| `controller` | `celestia-controller-host` | 把 `input.*` 和 tick 转换为 `model.*` command |
| `view` | `celestia-view-host` | 消费 `view.frame`，产生 `input.*`，Step6 第一轮为 Debug 2D / headless |
| `broadcast` | 虚拟目标 | 表示消息面向多个接收者，当前 smoke 中主要作为协议保留值 |

## 3. Envelope

所有跨进程消息都包在 `RuntimeEnvelope` 中。

| Field | Type | 说明 |
| --- | --- | --- |
| `protocolVersion` | int | 当前固定为 `1`。未知版本必须拒绝。 |
| `sessionId` | string | 一次 launcher 编排的会话标识。 |
| `sequenceId` | uint64 | 单调递增消息编号，用于排序和诊断。 |
| `timestampUs` | int64 | 发送时间，单位微秒。 |
| `sourceRole` | enum | `launcher` / `model` / `controller` / `view`。 |
| `targetRole` | enum | `launcher` / `model` / `controller` / `view` / `broadcast`。 |
| `kind` | enum | `lifecycle` / `command` / `event` / `viewFrame` / `heartbeat` / `error`。 |
| `name` | string | 具体消息名，例如 `runtime.start` 或 `model.step`。 |
| `payloadEncoding` | enum | 当前实现以 text payload 为主，保留 binary / none 扩展空间。 |
| `payload` | string / bytes | 消息负载。Step6 的 `ViewFrame` 使用轻量 DTO 序列化结果。 |

## 4. 消息族

### 4.1 Lifecycle

```text
runtime.hello
runtime.ready
runtime.start
runtime.pause
runtime.resume
runtime.shutdown
runtime.stopped
runtime.heartbeat
runtime.error
```

Step6 host loop 的最小状态机：

```text
Created
  runtime.hello -> Ready
Ready
  runtime.start -> Running
Running
  runtime.shutdown -> Stopped
Any state
  malformed / unsupported -> runtime.error
stdin EOF
  -> process exit
```

### 4.2 Controller -> Model Command

```text
model.setTime
model.setTimeScale
model.step
model.setPaused
model.selectObject
model.setObserver
model.requestSnapshot
```

Command 只表达意图，不携带 C++ 指针。Controller 不能直接读写 Model 内部对象。

### 4.3 Model -> View Frame / Event

```text
view.frame
view.selectionChanged
view.timeChanged
view.error
```

`view.frame` 第一轮使用 Step5 引入的 `ViewFrame` DTO：

```text
frameId
simulationTime
observerPosition
selections[]
summary
```

`ViewFrame` 不允许包含 `Simulation*`、`Universe*`、`Renderer*`、OpenGL context、texture、mesh、shader 等进程内资源。

### 4.4 View -> Controller Event

```text
input.key
input.mouse
input.resize
input.closeRequested
input.selectRequested
```

Step6 smoke 的 ViewService 以 Debug 2D / headless 为主，主要验证事件和 frame 消费语义。真实窗口输入事件流和 OpenGL 3D View 进程化应进入 Step7。

## 5. 方向矩阵

| Direction | Message | 当前状态 |
| --- | --- | --- |
| launcher -> model/controller/view | `runtime.hello`, `runtime.start`, `runtime.shutdown` | Step6 已实现 |
| model/controller/view -> launcher | `runtime.ready`, `runtime.stopped`, `runtime.error` | Step6 已实现 |
| controller -> model | `model.step`, `model.requestSnapshot`, `model.setPaused` | Step6 已实现最小闭环 |
| model -> view | `view.frame` | Step6 已实现 Debug 2D / headless 消费 |
| view -> controller | `input.*` | 协议已定义，Step6 仅做最小可测服务语义 |
| model/controller/view -> launcher | `runtime.heartbeat` | 协议已定义，后续可增强 supervisor 健康检查 |

## 6. Transport

### 6.1 Framing

当前实现使用 length-prefixed framed message：

```text
Content-Length: <bytes>

<serialized RuntimeEnvelope bytes>
```

Windows 上 host 在 `--serve` 模式会把 stdio 切到 binary mode，避免 CRLF 转换破坏 frame 长度。Step5 的 `--once` 文本握手继续保留。

### 6.2 Transport 替换原则

Transport 只能替换“字节如何传输”，不能改变协议语义。

```text
stdio / pipe / local socket / TCP 都必须传输同一个 RuntimeEnvelope。
shutdown、error、sequenceId、sessionId 的含义不能因 transport 改变。
高频大块数据可引入 shared memory 数据面，但控制面仍应使用 RuntimeEnvelope。
```

Step6 当前只承诺 framed stdio。named pipe、local socket、TCP、shared memory 都是后续扩展。

## 7. 当前 Smoke 拓扑

```text
celestia-sdl.exe
  ProcessSupervisor
    -> celestia-controller-host --serve
    -> celestia-model-host --serve
    -> celestia-view-host --serve
```

当前 supervisor 为了让测试确定性强，使用 framed stdio 输入 / 输出临时文件和脚本顺序驱动各 host。关键点是：每个 host 进入的是长期 `--serve` loop，直到收到 `runtime.shutdown` 才退出；不是 Step5 的一次性握手程序。

## 8. 已落地文件

| Area | Files |
| --- | --- |
| Protocol | `src/celruntime/protocol/envelope.*`, `lifecycle.h`, `payloadtype.h` |
| Transport | `src/celruntime/transport/framedmessage.*`, `framedtransport.h`, `stdiotransport.*` |
| Process | `src/celruntime/process/runtimehostloop.*`, `runtimehostcommon.*`, `processsupervisor.*` |
| Hosts | `modelhostmain.cpp`, `controllerhostmain.cpp`, `viewhostmain.cpp` |
| Services | `model/modelservice.*`, `controller/controllerservice.*`, `view/viewservice.*` |
| Tests | `test/unit/mvc_step6_*_test.cpp` |
| SDL entry | `src/celestia/sdl/sdlmain.cpp` |

## 9. 验收边界

Step6 可以作为以下结论的依据：

```text
MVC runtime protocol v1 已定义并有单元测试覆盖。
M / C / V host 支持长期 --serve lifecycle。
Debug 2D / headless 路径可以通过三个独立 host 进程完成本机 framed stdio 闭环。
现有 in-process OpenGL 3D 主路径保留，并通过启动截图验证未被 Step6 破坏。
```

Step6 不能作为以下结论的依据：

```text
OpenGL 3D View 已跨进程实时渲染。
任意 2D / 3D View 已能 DLL 热插拔。
M / C / V 已可跨机器部署。
shared memory 高吞吐数据面已经完成。
真实 Celestia SimulationBackend 已完整迁入独立 Model 进程。
```

## 10. 后续阶段约束

后续 Step7 / Step8 / Step9 必须复用本协议：

```text
Step7: 3D View 进程化时，View 进程持有窗口、OpenGL context 和 GPU 资源；Model 只发送 scene/view state。
Step8: View 插件 ABI / DLL hot reload 只能挂在协议和 runtime 装配层之后，不能让插件反向依赖 Model 内部对象。
Step9: shared memory 只作为数据面扩展，控制面仍使用 RuntimeEnvelope lifecycle / command / event。
```
