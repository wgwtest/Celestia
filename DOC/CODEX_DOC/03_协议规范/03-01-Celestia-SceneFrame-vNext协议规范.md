# Celestia SceneFrame vNext 协议规范

日期：2026-06-27

状态：Step12.1 增强版。本文是 Step13 真实 Model Backend、Step14 真实 SceneExtractor、Step15 View3D 消费真实场景、Step16 Controller 交互和 Step17 Resource/DataPlane 工作的输入规范。它不是 Step18 视觉保真的最终冻结版。

## 1. 目标与非目标

本文定义 Step12 的 `scene.frame vNext` 协议。它的目标是把后续真实 Model Backend、真实 SceneExtractor、跨进程 View3D、Controller 交互和 Resource/DataPlane 工作约束到同一个稳定输出协议上。

Step12 只冻结协议骨架、fixtures、validation 和 Model Lock Policy。它不加载真实 `Universe`，不把历史 Renderer 迁入 `celestia-view3d-host`，不声明跨进程 View3D 已经与历史 in-process Renderer 视觉等价。

## 2. 规范状态与约束强度

本文使用以下约束等级：

| 约束等级 | 含义 | 验收方式 |
| --- | --- | --- |
| 必须 | 后续阶段不得违反；违反即表示协议边界倒退 | 需要文档、fixture、unit test 或扫描脚本覆盖 |
| 应 | 默认遵守；如因真实 Celestia 数据结构限制无法遵守，必须在对应 Step 计划和机测记录中说明原因 | 需要人工审查和阶段机测记录 |
| 可以 | 可选扩展；不能破坏必须项和已发布字段语义 | 需要保持向后兼容 |

当前已经机器强制的约束包括：

```text
protocolVersion 只接受 1 或 SceneFrameProtocolVersion。
vNext 最小帧必须具备有效 camera、observer.frame 和 finite time.julianDayTdb。
ResourceRef.relativePath 必须是安全相对路径。
协议字段不得包含 Simulation* / Universe* / CelestiaCore* / Renderer* / Texture* / Mesh* / GLuint / 0x / :\ 等进程本地或平台本地 token。
协议头不得依赖 Renderer / OpenGL / SDL / Simulation / Universe 指针类型。
Step8 legacy scene.frame payload 必须继续可反序列化。
```

当前仍属于阶段验收强制、但尚未完全机器强制的约束包括：

```text
真实 Universe / Body / Star / Orbit / Timeline 语义不得为了 View3D 重写。
历史 Renderer 能力迁移时必须映射到 DTO、ResourceRef、data-plane 或 View 派生能力之一。
新增 scene.frame 字段必须给出 owner、单位、坐标/参考系语义、兼容规则和至少一个 fixture。
Controller 命令不得以 View 本地 pointer、OpenGL handle 或窗口资源作为 Model 输入。
```

## 3. 版本与扩展规则

`scene.frame vNext` 使用 `protocolVersion = 2`。后续 Step13-17 在不破坏已有字段含义的前提下，可以继续向 v2 追加字段。

必须遵守：

```text
已有字段不得删除、改名或改变单位/语义。
新增字段必须是 append-only；旧消费者应能忽略未知字段。
新增字段必须保持跨进程可序列化，不得携带本机指针、GPU 句柄、绝对路径或 Renderer 内部枚举裸值。
如果必须改变已有字段语义，必须升级到新的 protocolVersion，不能在 v2 内偷换语义。
新生产者必须输出 v2；v1 仅作为 Step8 legacy 读取兼容。
```

新增字段进入规范前，必须同时补齐：

```text
字段 owner：Model / SceneExtractor / Controller / View / Resource/DataPlane。
字段单位和坐标/参考系。
字段是否 required，以及缺省值含义。
valid fixture 和至少一个 invalid fixture 或边界测试。
validateSceneFrame 或阶段扫描中的对应校验，除非明确记录为人工验收项。
```

## 4. scene.frame vNext 总体结构

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

除字段名另有说明外，position、radius 和 orbit 采样点使用 Celestia Model / SceneExtractor 输出的公里尺度数值。velocity 字段在 Step14 真实投影前不得被 View3D 当作已冻结物理单位使用；Step14 必须补明确切单位。`observer.frame` 是解释这些数值的参考系名称；View3D 只能消费该参考系下的数值，不得通过访问 `Universe`、`Simulation` 或 `Observer` 自行重算场景状态。

## 5. 时间、相机、观察者字段

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

必须遵守：

```text
time.julianDayTdb 是 v2 消费者的主时间字段。
time.secondsSinceJ2000 是派生字段，必须与 julianDayTdb 对应同一时间点。
camera.position 和 observer.position 必须处于 observer.frame 所声明的参考系中。
camera.orientation 必须是可归一化四元数；真实接入阶段不得把 Renderer camera 对象或矩阵指针塞入协议。
```

## 6. Body / Star / DSO / Orbit / Label 字段

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

必须遵守：

```text
Body / Star / Orbit / DSO 的 objectId 是跨帧稳定逻辑 id，不是数组下标。
BodyRenderState.transform 是 Model/SceneExtractor 输出的只读投影；View3D 不得重新访问 Body / Timeline / Orbit 计算该矩阵。
BodyRenderState.radius 使用公里。
StarRenderState.position 是场景投影位置，不是 StarDatabase 内部对象地址。
OrbitRenderState.points 是已采样后的渲染折线；View3D 不得持有 Orbit* 或 Trajectory*。
LabelRenderState.targetObjectId 必须引用稳定 object id；空 target 只允许用于全局调试标签，不能用于真实对象标签。
```

## 7. ResourceRef 与 data-plane 预留

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

必须遵守：

```text
resource.id 在同一 frame 内必须稳定且唯一。
resource.kind 必须描述语义类型，例如 catalog、mesh、texture、material、shader-data、orbit-sample。
relativePath 表示仓库或数据根内的内容路径，不表示用户机器绝对路径。
contentHash 为空表示 Step12/13 尚未锁定内容完整性；Step17 资源阶段必须定义 required resource 的 hash 策略。
dataPlaneKey 为空表示该资源仍通过文件/fixture 或内联小 payload 表示；非空时必须由 Resource/DataPlane 文档定义生命周期和释放规则。
```

## 8. 稳定 object id 规则

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

object id 必须按以下优先级形成：

| 类型 | 推荐格式 | 说明 |
| --- | --- | --- |
| body | `celestia:body:Sol/Earth` | 使用稳定层级名或 catalog 可复现 id |
| star | `celestia:star:Sol` | 不得使用 StarDatabase 内存地址 |
| dso | `celestia:dso:M31` | 使用 catalog 名称、编号或可复现别名 |
| orbit | `celestia:orbit:Sol/Earth` | 表示已投影的 orbit stream，不是 `Orbit*` |
| resource | `res:texture:earth-day` | 表示资源引用，不是 GPU texture id |

真实接入阶段如果发现历史 Celestia 对象缺少稳定名字，必须在 SceneExtractor 中建立可复现 id 规则，并把规则补入本文。

## 9. Model Lock Policy

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

## 10. View3D 允许消费的边界

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

历史 View3D / Renderer 的算法、shader、resource utility 可以作为参考实现或可拆零件来源，但迁入跨进程 View3D 时必须满足：

```text
输入只能来自 scene.frame、ResourceRef、dataPlaneKey、View config 和 view.input。
允许保留 View 进程内部 GPU cache，但 cache key 必须来自 ResourceRef 或 View 自身配置。
不得要求 Model 进程暴露 TextureManager、GeometryManager、Renderer、Simulation、Universe 或 CelestiaCore。
```

## 11. Controller 后续命令与 scene.frame 状态关系

Controller 后续命令应以 `scene.frame` 中稳定 id 为对象引用，例如选择、跟随、缩放、时间控制、相机导航和 label visibility。Controller 不能把 View 端局部 pointer、OpenGL handle 或窗口资源传回 Model。

当前 Step12 只保留最小 view.input 到 Model 的回写能力；真实导航和选择命令属于后续 Step16/17。

## 12. 现有 Celestia 能力映射

本节用于避免“最小字段看起来可用，但覆盖不了既有系统能力”的问题。每个后续阶段都必须把新增能力归入下表之一。

| 既有能力 | 当前协议归属 | owner | 当前状态 | 后续约束 |
| --- | --- | --- | --- | --- |
| 时间、暂停、time scale | `TimeState` | Model / SceneExtractor | v2 已有最小字段 | Step13 必须从真实 Simulation/Timeline 投影，不得由 View 推导 |
| 观察者、相机、参考系 | `ObserverState`, `CameraState` | Model / SceneExtractor | v2 已有最小字段 | Step14 必须明确 frame、单位、姿态约定 |
| 选择对象 | `SelectionState` | Controller / Model | v2 已有 id/type | Step16 命令必须使用稳定 id，不得传 pointer |
| 太阳系天体、半径、姿态、可见性 | `BodyRenderState` | Model / SceneExtractor | v2 有骨架，真实投影未完成 | Step14 必须从真实 Body/Timeline/Frame 投影；View 不计算天体语义 |
| 恒星、星表、星等、颜色 | `StarRenderState` | Model / SceneExtractor | v2 有骨架，真实 catalog 未完成 | Step14 必须定义星表裁剪、亮度和颜色来源 |
| 深空对象、星云、星团、星系 | `DeepSkyRenderState` | Model / SceneExtractor | v2 有最小 DSO 字段 | Step14 必须映射 DSO catalog id、类型和位置 |
| 轨道和路径曲线 | `OrbitRenderState.points` | SceneExtractor | v2 有采样折线 | Step14/15 必须定义采样精度、时间窗口和单位 |
| label、名称、对象标注 | `LabelRenderState` | Model / Controller / View | v2 已结构化 | Step16 必须定义 label visibility 命令和对象引用 |
| mesh、texture、catalog、material | `ResourceRef` / data-plane | Resource/DataPlane | v2 有最小引用 | Step17 必须定义 hash、生命周期、required 语义和数据面传输 |
| render setting、显示开关 | `RenderSettings` | Controller / View config / Model | v2 仅有 stars/orbits/labels/ambient/exposure | Step15/16 必须区分 Model 输出、Controller 命令和 View 本地配置 |
| 输入事件、导航、选择命令 | 不属于 `scene.frame`，通过 `view.input` / controller command | Controller | Step12 只有最小回写 | Step16 必须定义命令协议，且命令只能引用稳定 id |

## 13. 未覆盖历史能力的处理门槛

以下历史能力在当前 v2 中尚未完整表达。它们不是“被放弃”，而是进入后续阶段时必须显式分类。

| 能力 | 当前处理 | 后续必须选择的归属 |
| --- | --- | --- |
| atmosphere / scattering / cloud | 未纳入 v2 结构 | BodyRenderState 扩展、ResourceRef、或 View 派生；必须说明光照和资源来源 |
| ring system | 未纳入 v2 结构 | BodyRenderState 扩展或 ResourceRef；不得传 RingSystem* |
| eclipse / shadow | 未纳入 v2 结构 | SceneExtractor 输出事件/几何，或 View 派生规则；必须说明时间和遮挡来源 |
| constellation / asterism / boundaries | 未纳入 v2 结构 | DSO/overlay 扩展或独立 line layer；不得传 AsterismList* |
| grids / reference marks / axis arrows | 未纳入 v2 结构 | View config、Controller command 或 scene overlay；必须说明是否属于 Model 语义 |
| overlays / console / UI / script menu | 不属于 scene.frame | Frontend/View 本地能力或 Controller/UI 协议；不得污染 Model 输出 |
| high precision curve rendering / LOD | v2 只有 sampled orbit points | View 内部算法可以保留，但输入必须来自协议采样或 ResourceRef |

后续实现如果需要上述能力，必须先更新本文或新增同目录协议规范，再改代码。不得把历史 Renderer 对象直接穿透到 Model / Runtime 协议层。

## 14. 验证与增强门槛

Step13-17 每次增强 `scene.frame` 都必须满足：

```text
1. 协议规范先更新，写明字段 owner、单位、参考系、默认值和兼容规则。
2. sceneprotocol.h/cpp 或对应协议文件增加字段和序列化/反序列化。
3. validateSceneFrame 增加可机器检查的约束；无法机器检查的部分写入机测记录。
4. fixtures 至少包含一个 valid 示例；风险字段必须包含 invalid 示例。
5. Step8 legacy 反序列化测试继续通过。
6. protocol header 继续保持 renderer/OpenGL/SDL/Simulation/Universe free。
7. Full 兼容回归仍以普通统一 exe 作为原能力不退化参照。
```

如果某阶段只更新实现而未更新规范，应视为协议治理失败，不能进入 Model 层整体冻结。

## 15. Step12 可以说和不能说

Step12 完成后可以说：

```text
scene.frame vNext 协议已经定义并由自动测试验证。
Model 输出边界已具备 protocolVersion、TimeState、ResourceRef vNext、结构化 label、DSO 预留和 validation helper。
Step12.1 已补充约束强度、现有 Celestia 能力映射、未覆盖历史能力处理门槛和后续增强门槛。
Step13-17 必须以该协议作为 Model/View/Controller/Resource 边界。
```

Step12 完成后不能说：

```text
真实 Celestia Universe 已经运行在独立 Model host 中。
跨进程 View3D 已经达到历史 Renderer 视觉等价。
真实 catalog、mesh、texture 和 orbit 资源已经全部走最终 data-plane。
```
