# Celestia SceneFrame vNext 协议规范

日期：2026-06-27

## 1. 目标与非目标

本文定义 Step12 的 `scene.frame vNext` 协议。它的目标是把后续真实 Model Backend、真实 SceneExtractor、跨进程 View3D、Controller 交互和 Resource/DataPlane 工作约束到同一个稳定输出协议上。

Step12 只冻结协议骨架、fixtures、validation 和 Model Lock Policy。它不加载真实 `Universe`，不把历史 Renderer 迁入 `celestia-view3d-host`，不声明跨进程 View3D 已经与历史 in-process Renderer 视觉等价。

## 2. scene.frame vNext 总体结构

`scene.frame vNext` 的协议版本为：

```cpp
inline constexpr int SceneFrameProtocolVersion{ 2 };
```

顶层结构包含：

```text
protocolVersion
sessionId
sequence
simulationTime
time
camera
observer
renderSettings
resources
bodies
stars
deepSkyObjects
orbits
labels
selection
```

`simulationTime` 保留为 Step8 legacy 兼容字段。vNext 消费者应优先读取 `time.julianDayTdb`。

## 3. 时间、相机、观察者字段

时间字段为：

```cpp
struct TimeState
{
    double julianDayTdb{ 0.0 };
    double secondsSinceJ2000{ 0.0 };
    double timeScale{ 1.0 };
    bool paused{ false };
};
```

相机字段继续使用 `CameraState`，最小有效帧要求：

```text
camera.fov > 0
camera.nearPlane > 0
camera.farPlane > camera.nearPlane
```

观察者字段继续使用 `ObserverState`，vNext 最小要求 `observer.frame` 非空。后续 Step13/14 接入真实 Model Backend 时，应把真实 Observer 参考系、位置和速度映射到该结构，而不是让 View 端自行推导 Celestia 时间或观察者状态。

## 4. Body / Star / DSO / Orbit / Label 字段

Body、Star、Orbit 均保留 legacy id 字段，同时增加稳定逻辑 object id：

```text
BodyRenderState.objectId
StarRenderState.objectId
OrbitRenderState.objectId
```

推荐 object id 格式：

```text
celestia:body:Sol/Earth
celestia:star:Sol
celestia:dso:M31
celestia:orbit:Sol/Earth
```

DSO 最小结构为：

```cpp
struct DeepSkyRenderState
{
    std::string objectId;
    std::string name;
    std::string dsoType;
    std::array<double, 3> positionKm{ 0.0, 0.0, 0.0 };
    double apparentMagnitude{ 0.0 };
    bool visible{ true };
    ResourceRef catalogResource;
};
```

Label 从 `std::string` 升级为结构化状态：

```cpp
struct LabelRenderState
{
    std::string targetObjectId;
    std::string text;
    std::string kind;
    bool visible{ true };
};
```

反序列化仍兼容 Step8 的 `labelN=Earth`，并映射到 `LabelRenderState.text`。

## 5. ResourceRef 与 data-plane 预留

ResourceRef vNext 为：

```cpp
struct ResourceRef
{
    std::string id;
    std::string kind;
    std::string package;
    std::string relativePath;
    std::string contentHash;
    std::string dataPlaneKey;
    bool required{ false };
};
```

`relativePath` 必须是相对内容根的安全路径，不能是 Windows drive path，不能以 `/` 或 `\` 开头，不能包含 `..` traversal。`dataPlaneKey` 在 Step12 可以为空，后续 Step14/15 用于连接共享内存或其它数据面对象。

## 6. 稳定 object id 规则

object id 不得为空，不得包含进程本地指针语义，不得使用数组下标或本机句柄。以下 token 禁止出现在协议字段中：

```text
Simulation*
Universe*
CelestiaCore*
Renderer*
Texture*
Mesh*
GLuint
0x
:\
```

该规则确保 Model 输出可以跨进程、跨帧稳定消费。

## 7. Model Lock Policy

结构锁已经由 Step3 / Step4 生效。核心语义锁从 Step12 开始执行，到 Step14 完成后基本成立。

Step12-14 允许修改：

```text
scene.frame 输出协议
SceneExtractor
ModelService facade
只读投影层
ResourceRef / data-plane 边界
```

Step12-14 禁止为了 View 改写：

```text
Universe
Body
Star
Orbit
Timeline
历史天文语义
```

Model 层整体冻结只能在 Step17 后进入。Step12 的锁死含义是：核心能力不变，只扩展输出协议；不是 Model 源码完全不再变化。

## 8. View3D 允许消费的边界

跨进程 View3D 只能消费 `scene.frame` 中的 DTO、ResourceRef 和后续 data-plane key。它不得直接持有或要求：

```text
Simulation*
Universe*
Renderer*
Texture*
Mesh*
GLuint
```

Step12 后，View3D 可以根据 `camera`、`observer`、`bodies`、`stars`、`deepSkyObjects`、`orbits`、`labels`、`resources` 渲染或验证帧，但不能回穿 Model 内部对象。

## 9. Controller 后续命令与 scene.frame 状态关系

Controller 后续命令应以 `scene.frame` 中稳定 id 为对象引用，例如选择、跟随、缩放、时间控制、相机导航和 label visibility。Controller 不能把 View 端局部 pointer、OpenGL handle 或窗口资源传回 Model。

当前 Step12 只保留最小 view.input 到 Model 的回写能力；真实导航和选择命令属于后续 Step16/17。

## 10. Step12 可以说和不能说

Step12 完成后可以说：

```text
scene.frame vNext 协议已经定义并由自动测试验证。
Model 输出边界已具备 protocolVersion、TimeState、ResourceRef vNext、结构化 label、DSO 预留和 validation helper。
Step13-17 必须以该协议作为 Model/View/Controller/Resource 边界。
```

Step12 完成后不能说：

```text
真实 Celestia Universe 已经运行在独立 Model host 中。
跨进程 View3D 已经达到历史 Renderer 视觉等价。
真实 catalog、mesh、texture 和 orbit 资源已经全部走最终 data-plane。
```
