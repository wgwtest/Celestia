# Celestia 标准 MVC 解耦与迁移映射说明

## 1. 文档目的

本文记录 Celestia 本仓库 Step 1 的代码级 MVC 解耦结果，并为后续 Planet_SIM clean-room 迁移提供类来源和剥离边界。Step 1 的完成口径是 Celestia 源码内完成 Model / Controller / View 边界收缩、构建出可运行程序、全量测试通过；Step 2 的 UE 插件迁移与本步骤独立。

## 2. Celestia 源码参考快照

当前实现分支为 `codex/celestia-mvc-step1`，工作区为 `D:/WorkSpace/Codex/CeleNew/.worktrees/celestia-mvc-step1`。参考主链为：

```text
Catalog loaders
-> Universe
-> StarDatabase / SolarSystemCatalog / DSODatabase
-> SolarSystem / PlanetarySystem
-> Body / Star / DeepSkyObject / Location
-> Timeline / Orbit / RotationModel / ReferenceFrame
-> Simulation / Observer / Selection
-> View Adapter
-> Renderer
```

## 3. MVC 解耦规则

| 层 | 规则 |
| --- | --- |
| Model | 保存天体、目录、轨道、姿态、层级、可选特征和查找语义；模型头文件不暴露 `Renderer`、`RenderFlags`、`GeometryManager` 或主渲染资产句柄 |
| Controller | 负责时间、观察者、选择、导航和运行态状态推进；不转发渲染，不持有拾取所需的 View 资源 |
| View Adapter | 在模型数据和 View 资源之间做转换，例如拾取、渲染资产读取、位置几何投影和渲染标志策略 |
| View | `Renderer`、`TextureManager`、`GeometryManager`、OpenGL、SDL/Win/Qt 前端、贴图和网格加载 |
| Application Shell | `CelestiaCore` 编排输入、模拟、渲染和 View Adapter，不作为 Planet_SIM Core 直接迁移对象 |

### 3.1 分层依赖总图

下图中的箭头表示 Step 1 后允许保留的编译依赖或运行时编排方向。Model / Controller 的 public header 不得反向暴露 View 资源；需要 View 资源的行为统一落在 View Adapter 或 Application Shell。

```mermaid
flowchart LR
    catalog["Catalog loaders"] --> universe["Universe / catalogs"]
    universe --> model["Model objects<br/>Body / Star / DSO / Location"]
    dynamics["Orbit / RotationModel / ReferenceFrame"] --> model

    simulation["Simulation<br/>Controller"] --> universe
    simulation --> state["Selection / Observer"]

    shell["CelestiaCore<br/>Application Shell"] --> simulation
    shell --> renderer["Renderer<br/>View"]
    shell --> picker["SelectionPicker<br/>View Adapter"]

    picker --> universe
    picker --> geometry["GeometryManager<br/>View resource"]
    renderer --> bodyAssets["BodyRenderAssets"]
    renderer --> starAssets["StarRenderAssets"]
    renderer --> nebulaAssets["NebulaRenderAssets"]
    renderer --> dsoPolicy["DeepSkyObjectRenderPolicy"]

    bodyAssets --> model
    starAssets --> model
    nebulaAssets --> model
    dsoPolicy --> model

    viewResources["Renderer / GeometryManager / TextureManager / RenderFlags"] -. "不得进入" .-> coreHeaders["Model / Controller public headers"]
```

### 3.2 核心类依赖变化图

本图只表达本次 Step 1 拆除和迁出的关键依赖，不等同于完整 C++ include graph。左侧是原始混杂点，右侧是更新后的边界落点。

```mermaid
flowchart TB
    subgraph before["Before: mixed MVC dependencies"]
        oldSimulation["Simulation"] --> oldRenderer["Renderer / RenderFlags / pickObject"]
        oldUniverse["Universe"] --> oldPicking["GeometryManager / pickPlanet / pickStar / pickDeepSkyObject"]
        oldBody["Body"] --> oldBodyAssets["GeometryHandle / Surface / alternate surface / ring texture"]
        oldStar["Star / StarDetails"] --> oldStarAssets["TextureHandle / GeometryHandle / StarTextureSet"]
        oldDso["DeepSkyObject subclasses"] --> oldDsoView["RenderFlags / RenderLabels / pick / GeometryHandle"]
    end

    subgraph after["After: Step 1 boundaries"]
        newSimulation["Simulation"] --> controllerState["time / selection / observer / tracking"]
        newUniverse["Universe"] --> catalogs["StarDatabase / SolarSystemCatalog / DSODatabase"]
        newShell["CelestiaCore"] --> newRenderer["Renderer"]
        newShell --> newPicker["SelectionPicker"]
        newPicker --> newUniverse
        newPicker --> newGeometry["GeometryManager"]
        bodyAdapter["BodyRenderAssets"] --> newBody["Body"]
        starAdapter["StarRenderAssets"] --> newStar["Star / StarDetails"]
        nebulaAdapter["NebulaRenderAssets"] --> newNebula["Nebula"]
        dsoRenderPolicy["DeepSkyObjectRenderPolicy"] --> newDso["DeepSkyObject"]
        dsoPicker["DeepSkyObjectPicker"] --> newDso
    end
```

### 3.3 文件依赖落点图

下图用于审查“新增文件是否真的承接了原先散落在 Model / Controller 中的 View 依赖”。实线表示 include 或直接调用方向；虚线表示 header 中只保留前向声明或友元入口，不暴露具体 View 资源类型。

```mermaid
flowchart LR
    celestiaCore["src/celestia/celestiacore.cpp"] --> selectionPickerH["src/celengine/selectionpicker.h"]
    celestiaCore --> bodyAssetsH["src/celengine/bodyrenderassets.h"]

    renderCpp["src/celengine/render.cpp"] --> bodyAssetsH
    renderCpp --> starAssetsH["src/celengine/starrenderassets.h"]
    renderCpp --> nebulaAssetsH["src/celengine/nebularenderassets.h"]
    renderCpp --> dsoPolicyH["src/celengine/deepskyobjectrenderpolicy.h"]

    selectionPickerCpp["src/celengine/selectionpicker.cpp"] --> bodyAssetsH
    selectionPickerCpp --> dsoPickerH["src/celengine/deepskyobjectpicker.h"]
    selectionPickerCpp --> geometryManager["src/celengine/meshmanager.h / GeometryManager"]

    solarsysCpp["src/celengine/solarsys.cpp"] --> bodyAssetsH
    stardbBuilderCpp["src/celengine/stardbbuilder.cpp"] --> starAssetsH
    nebulaCpp["src/celengine/nebula.cpp"] --> nebulaAssetsH
    bodyCpp["src/celengine/body.cpp"] --> bodyAssetsH
    starCpp["src/celengine/star.cpp"] --> starAssetsH

    bodyH["src/celengine/body.h"] -. "forward friend only" .-> bodyAssetsH
    bodyH -. "forward friend only" .-> projectorH["src/celengine/bodylocationgeometryprojector.h"]
    starH["src/celengine/star.h"] -. "forward friend only" .-> starAssetsH
```

### 3.4 接口关系与迁移边界图

Step 2 只能迁移 Core 语义和接口关系，不能把 Celestia 的 GPL 实现代码、OpenGL 渲染管线或平台前端搬进 Planet_SIM。View Adapter 的价值是提供边界模式，而不是作为 Planet_SIM Core 类型直接复制。

```mermaid
flowchart LR
    celestiaModel["Celestia Model semantics<br/>Universe / Body / Star / Orbit / RotationModel / ReferenceFrame"] --> planetCore["Planet_SIM Core clean-room<br/>FCelestialUniverse / FCelestialBody / FCelestialStar / ICelestialOrbit"]
    celestiaController["Celestia Controller semantics<br/>Simulation / Selection / Observer"] --> planetCore

    celestiaAdapter["Celestia View Adapter pattern<br/>SelectionPicker / RenderAssets / Projector / RenderPolicy"] --> planetAdapter["Planet_SIM View Adapter<br/>UE / Cesium / DigitalPlanet / Inspector"]
    planetCore --> planetAdapter

    celestiaView["Celestia View implementation<br/>Renderer / TextureManager / GeometryManager / SDL / Qt / Win / OpenGL"] --> noMigrate["Do not migrate into Core"]
    celestiaView -. "only capability reference" .-> planetAdapter
```

| 接口边界 | 允许迁移 | 不允许迁移 |
| --- | --- | --- |
| Core Model | 对象关系、目录聚合、路径查找、轨道/姿态查询语义 | `GeometryManager`、`TextureHandle`、OpenGL 资源句柄 |
| Core Controller | 时间推进、选择、观察者、跟踪状态语义 | `Renderer` 转发、screen-space picking、平台输入事件 |
| View Adapter | 资源访问模式、拾取适配模式、Core 输出到 View 的转换模式 | 作为 Core 类型直接复制，或让 Adapter 反向污染 Core header |
| View | 只作为能力和验证参考 | 渲染管线、窗口系统、shader、buffer、具体 GPL 实现代码 |

## 4. Model 层类表

| Celestia 类 | Source files | Planet_SIM target | 迁移级别 |
| --- | --- | --- | --- |
| `Universe` | `src/celengine/universe.h/.cpp` | `FCelestialUniverse` | 接口语义和对象聚合 clean-room 迁移 |
| `StarDatabase` / `StarCatalog` | `src/celengine/stardb.h/.cpp` | `FCelestialStarCatalog` | 目录语义迁移 |
| `SolarSystemCatalog` / `SolarSystem` / `PlanetarySystem` | `src/celengine/solarsys.h/.cpp`, `src/celengine/body.h/.cpp` | `FCelestialSolarSystem`, `FCelestialPlanetarySystem` | 层级和查找语义迁移 |
| `Body` | `src/celengine/body.h/.cpp` | `FCelestialBody` | 物理、分类、轨道姿态查询语义迁移 |
| `Star` / `StarDetails` | `src/celengine/star.h/.cpp` | `FCelestialStar` | 光度、谱型、轨道关系语义迁移 |
| `Location` | `src/celengine/location.h/.cpp` | `FCelestialLocation` | 表面位置语义迁移 |
| `Timeline` / `TimelinePhase` | `src/celengine/timeline.h/.cpp`, `src/celengine/timelinephase.h/.cpp` | `FCelestialTimeline`, `FCelestialTimelinePhase` | 时间段状态源语义迁移 |
| `Orbit` | `src/celephem/orbit.h/.cpp` | `ICelestialOrbit` | 接口语义迁移 |
| `RotationModel` | `src/celephem/rotation.h/.cpp` | `ICelestialRotationModel` | 接口语义迁移 |
| `ReferenceFrame` | `src/celengine/frame.h/.cpp` | `ICelestialReferenceFrame` | 坐标 frame 语义迁移 |
| `BodyFeaturesManager` | `src/celengine/body.h/.cpp` | `FCelestialBodyFeatureStore` | 可选特征外挂模式迁移 |

## 5. Controller 层类表

| Celestia 类 | Source files | Planet_SIM target | 迁移级别 |
| --- | --- | --- | --- |
| `Simulation` | `src/celengine/simulation.h/.cpp` | `FCelestialSimulation` | 时间、选择、观察者和跟踪状态 clean-room 迁移 |
| `Selection` | `src/celengine/selection.h/.cpp` | `FCelestialSelection` | 选择 union 语义迁移 |
| `Observer` / `ObserverFrame` | `src/celengine/observer.h/.cpp` | `FCelestialObserver`, `FCelestialObserverFrame` | 观察者状态和参考 frame 语义迁移 |
| `CatalogLoader` | `src/celestia/catalogloader.h/.cpp` | `FCelestialCatalogLoader` | 数据加载流程参考 |
| `StarDatabaseBuilder` / SSO loaders | `src/celengine/stardbbuilder.*`, `src/celestia/load*.cpp` | `FCelestialCatalogBuilder` | 构建流程参考 |

## 6. View 层类表

| Celestia 类 | 处理 |
| --- | --- |
| `Renderer` | 不迁移到 Core；只作为 Planet_SIM View Adapter 的能力参考 |
| `TextureManager` | 不迁移到 Core；UE 侧由资产系统或材质系统承接 |
| `GeometryManager` | 不迁移到 Core；UE / Cesium / DigitalPlanet 侧承接网格与拾取资源 |
| `RenderFlags` / `RenderLabels` | 不进入 Core；只在 View Adapter 层映射 |
| SDL / Win / Qt 前端 | 不迁移 UI 事件和窗口系统 |

## 7. Mixed 类拆分表

| Mixed 点 | 已落地拆分 |
| --- | --- |
| `Universe` 持有 `GeometryManager` 并实现拾取 | 拆出 `SelectionPicker`，`Universe` 头文件移除 `GeometryManager`、`RenderFlags` 和 `pick*` |
| `Simulation` 转发渲染和拾取 | 删除 `render` / `pickObject`，由 `CelestiaCore` 编排 `Renderer` 与 `SelectionPicker` |
| `BodyFeaturesManager::computeLocations` 依赖几何管理器 | 拆出 `BodyLocationGeometryProjector` |
| `Body` 暴露主几何、主表面、alternate surface 和 rings 纹理资产 | 拆出 `BodyRenderAssets`，渲染、拾取、前端菜单和 SSO 加载器通过适配层读写 |
| `Star` / `StarDetails` 暴露纹理和网格句柄 | 拆出 `StarRenderAssets`，保留 `StarDetails` copy-on-write 语义 |
| `Nebula` 暴露网格句柄 | 拆出 `NebulaRenderAssets` |
| `DeepSkyObject` 子类暴露 render mask 和拾取 | 拆出 `DeepSkyObjectRenderPolicy` 与 `DeepSkyObjectPicker` |

## 8. Planet_SIM 迁移类表

| Planet_SIM 类型 | Celestia 来源 | Core 边界 |
| --- | --- | --- |
| `FCelestialUniverse` | `Universe` | 聚合目录、路径查找、对象查找 |
| `FCelestialBody` | `Body` | 物理量、分类、父子关系、位置姿态查询 |
| `FCelestialStar` | `Star` / `StarDetails` | 星体光度、谱型、轨道关系 |
| `FCelestialSelection` | `Selection` | Body / Star / Location 等选择语义 |
| `FCelestialSimulation` | `Simulation` | 时间推进、当前选择、观察者、跟踪 |
| `ICelestialOrbit` | `Orbit` | `positionAtTime` / `velocityAtTime` 语义 |
| `ICelestialRotationModel` | `RotationModel` | 自转姿态和角速度语义 |
| `ICelestialReferenceFrame` | `ReferenceFrame` / `ObserverFrame` | 坐标转换和参考对象语义 |

## 9. 不迁移能力表

| 能力 | 原因 |
| --- | --- |
| `Renderer` 渲染管线 | 属于 View，不进入 Planet_SIM Core |
| `TextureManager` / `GeometryManager` | 属于资源和渲染资产管理，由 UE 资产系统或 View Adapter 承接 |
| `RenderFlags` / `RenderLabels` | 属于显示策略，由 View Adapter 转换 |
| SDL / Win / Qt 窗口事件 | 平台 UI，不进入 Core |
| OpenGL shader、buffer、framebuffer | View 实现细节 |

## 10. 命名一致性规则

| Celestia 命名 | Planet_SIM 命名 |
| --- | --- |
| `Universe` | `FCelestialUniverse` |
| `Body` | `FCelestialBody` |
| `Star` | `FCelestialStar` |
| `Selection` | `FCelestialSelection` |
| `Simulation` | `FCelestialSimulation` |
| `Orbit` | `ICelestialOrbit` |
| `RotationModel` | `ICelestialRotationModel` |
| `ReferenceFrame` | `ICelestialReferenceFrame` |

## 11. 许可证与 clean-room 执行规则

Celestia 源码用于架构和语义参考。迁移到 Planet_SIM 时不得逐字搬运 GPL 实现代码；只迁移接口语义、对象关系、状态机和测试可验证行为。UE 插件内核使用新的类型、命名空间和实现，并用自动化测试证明语义等价。

## 12. 对后续代码实现的强制门槛

1. Core 头文件不得引用 `AActor`、`UWorld`、Cesium、DigitalPlanet、`UMaterial` 或 `UStaticMesh`。
2. `FCelestialUniverse` 必须能由 catalog entries 构建长期存在的对象图。
3. `FCelestialUniverse::FindPath("Sol/Earth")` 必须返回 Body selection。
4. `FCelestialSimulation` 必须能设置选择、推进时间、跟踪对象并构建快照。
5. View Adapter 可以消费 Core 输出，但不得反向污染 Core 模型。
6. 每个迁移类必须有对应 Celestia 来源和自动化测试。

## 13. Step 1 代码验收记录

本仓库已新增 `test/unit/mvc_boundary_test.cpp`，覆盖：

- `Simulation` 头文件不暴露 `Renderer`、`RenderFlags`、`render`、`pickObject`
- `Universe` 头文件不暴露 `GeometryManager`、`RenderFlags`、`pick*`
- `BodyFeaturesManager` 头文件不暴露 `GeometryManager` 和 `computeLocations`
- `Body` 头文件不暴露主几何、主表面、alternate surface、ring texture、`TextureHandle` 和 `meshmanager.h`
- `Star` 头文件不暴露纹理、网格和 `meshmanager.h`
- DeepSkyObject 模型头文件不暴露 `RenderFlags`、`RenderLabels`、render mask、拾取、`Renderer` 或 `GeometryHandle`

验证结果：

```text
build-mvc-baseline-rel: cmake --build --parallel 8 passed
build-mvc-baseline-rel: ctest --output-on-failure passed, 42/42
build-mvc-sdl-rel: cmake --build --parallel 8 passed
build-mvc-sdl-rel: ctest --output-on-failure passed, 42/42
build-mvc-sdl-rel/src/celestia/sdl/celestia-sdl.exe built and entered the SDL run loop with a minimal runtime catalog
```

## 14. 详细类映射卡

### Celestia class: `CelestiaCore`

Source files:

```text
src/celestia/celestiacore.h
src/celestia/celestiacore.cpp
```

MVC role: Application Shell。

Keep in Planet_SIM Core: 不直接迁移；仅参考应用层如何编排 catalog、simulation、selection、observer 与 renderer。

Strip from Planet_SIM Core: UI 事件、前端窗口、OpenGL/SDL/Qt/Win 细节、`Renderer` 生命周期。

Planet_SIM target: View Adapter / Editor Shell 参考，不是 Core 类型。

Migration level: 编排语义参考。

Validation: Celestia 内由 `CelestiaCore` 直接编排 `Renderer` 与 `SelectionPicker`，不再要求 `Simulation` 转发渲染或拾取。

### Celestia class: `Universe`

Source files:

```text
src/celengine/universe.h
src/celengine/universe.cpp
```

MVC role: Model aggregate。

Keep in Planet_SIM Core: 星表、太阳系目录、DSO 目录、对象查找、路径查找、selection resolving support。

Strip from Planet_SIM Core: `GeometryManager` ownership、`RenderFlags`、renderer-specific picking implementation、close-star picking cache。

Planet_SIM target: `FCelestialUniverse`。

Migration level: Interface and structure migration, clean-room implementation。

Validation: `Universe` 头文件不暴露 `GeometryManager`、`RenderFlags` 或 `pick*`，拾取已迁入 `SelectionPicker`。

### Celestia class: `Body`

Source files:

```text
src/celengine/body.h
src/celengine/body.cpp
```

MVC role: Model with former render assets。

Keep in Planet_SIM Core: 物理量、分类、父子关系、轨道姿态查询、可点击/可见运行态语义、生命周期区间。

Strip from Planet_SIM Core: primary geometry、primary surface、alternate surface、rings texture、mesh manager dependency、location mesh projection。

Planet_SIM target: `FCelestialBody`。

Migration level: Interface semantics migration, clean-room implementation。

Validation: `Body` 头文件不暴露 `meshmanager.h`、`GeometryHandle` 主字段、`TextureHandle`、primary `Surface` 字段或 alternate surface API，渲染资产经 `BodyRenderAssets` 访问。

### Celestia class: `Star` / `StarDetails`

Source files:

```text
src/celengine/star.h
src/celengine/star.cpp
```

MVC role: Model with former render assets。

Keep in Planet_SIM Core: 光度、半径、温度、谱型、轨道、可见运行态语义。

Strip from Planet_SIM Core: texture、mesh geometry、star texture set。

Planet_SIM target: `FCelestialStar`。

Migration level: Interface semantics migration, clean-room implementation。

Validation: `Star` 头文件不暴露 `TextureHandle`、`GeometryHandle`、`StarTextureSet` 或 texture/geometry getter/setter，渲染资产经 `StarRenderAssets` 访问。

### Celestia class: `DeepSkyObject` / `Galaxy` / `Globular` / `Nebula` / `OpenCluster`

Source files:

```text
src/celengine/deepskyobj.h/.cpp
src/celengine/galaxy.h/.cpp
src/celengine/globular.h/.cpp
src/celengine/nebula.h/.cpp
src/celengine/opencluster.h/.cpp
```

MVC role: Model with former render policy and picking methods。

Keep in Planet_SIM Core: 名称、位置、半径、绝对星等、点击/可见运行态语义、对象类型。

Strip from Planet_SIM Core: render mask、label mask、renderer picking、nebula geometry handle。

Planet_SIM target: 当前迁移优先级低；如迁移，进入 `FCelestialDeepSkyObject` 族。

Migration level: Optional semantic migration, clean-room implementation。

Validation: DSO 模型头文件不暴露 `RenderFlags`、`RenderLabels`、`pick` 或 `GeometryHandle`，策略和拾取分别迁入 `DeepSkyObjectRenderPolicy` / `DeepSkyObjectPicker`。

### Celestia class: `Simulation`

Source files:

```text
src/celengine/simulation.h
src/celengine/simulation.cpp
```

MVC role: Controller。

Keep in Planet_SIM Core: 时间推进、选择、观察者、跟踪、暂停和速度控制。

Strip from Planet_SIM Core: `Renderer` 转发、`RenderFlags`、screen-space picking。

Planet_SIM target: `FCelestialSimulation`。

Migration level: Controller semantics migration, clean-room implementation。

Validation: `Simulation` 头文件不暴露 `Renderer`、`RenderFlags`、`render` 或 `pickObject`。

### Celestia class: `Selection` / `Observer` / `ObserverFrame`

Source files:

```text
src/celengine/selection.h/.cpp
src/celengine/observer.h/.cpp
src/celengine/frame.h/.cpp
```

MVC role: Controller state and frame model。

Keep in Planet_SIM Core: typed selection、observer state、tracking/chase/follow semantics、reference-frame transform semantics。

Strip from Planet_SIM Core: UI command wiring and renderer state。

Planet_SIM target: `FCelestialSelection`、`FCelestialObserver`、`ICelestialReferenceFrame`。

Migration level: Interface semantics migration, clean-room implementation。

Validation: Step 2 中 `FCelestialSimulation` 必须能设置选择、跟踪对象并由 observer frame 构建快照；Step 1 只保留来源和边界。

### Celestia class group: catalogs and hierarchy

Source files:

```text
src/celengine/stardb.h/.cpp
src/celengine/stardbbuilder.h/.cpp
src/celengine/solarsys.h/.cpp
src/celengine/timeline.h/.cpp
src/celengine/timelinephase.h/.cpp
src/celengine/location.h/.cpp
```

MVC role: Model plus catalog-building controller。

Keep in Planet_SIM Core: star catalog aggregation、solar-system hierarchy、timeline phases、location semantics、catalog entry to object graph build flow。

Strip from Planet_SIM Core: Celestia file syntax coupling、texture and mesh resolution、frontend-specific categories。

Planet_SIM target: `FCelestialStarCatalog`、`FCelestialSolarSystem`、`FCelestialPlanetarySystem`、`FCelestialTimeline`、`FCelestialLocation`、`FCelestialCatalogLoader`。

Migration level: Structure and behavior migration, clean-room implementation。

Validation: Step 2 中 `FCelestialUniverse` 必须能由 catalog entries 构建对象图并解析 `FindPath("Sol/Earth")`。

### Celestia class group: dynamics

Source files:

```text
src/celephem/orbit.h/.cpp
src/celephem/rotation.h/.cpp
src/celengine/frame.h/.cpp
```

MVC role: Model service interfaces。

Keep in Planet_SIM Core: position/velocity query、rotation query、reference-frame transform semantics。

Strip from Planet_SIM Core: Celestia implementation代码和 GPL 具体算法复制。

Planet_SIM target: `ICelestialOrbit`、`ICelestialRotationModel`、`ICelestialReferenceFrame`。

Migration level: Interface semantics migration only。

Validation: Step 2 使用 UE 自动化测试证明 fixed、sampled 或 simple analytic implementation 的确定性。

### Celestia class group: View and View Adapter

Source files:

```text
src/celengine/render.h/.cpp
src/celengine/texmanager.h/.cpp
src/celengine/meshmanager.h/.cpp
src/celengine/selectionpicker.h/.cpp
src/celengine/bodyrenderassets.h/.cpp
src/celengine/starrenderassets.h/.cpp
src/celengine/nebularenderassets.h/.cpp
src/celengine/bodylocationgeometryprojector.h/.cpp
src/celengine/deepskyobjectrenderpolicy.h/.cpp
```

MVC role: View or View Adapter。

Keep in Planet_SIM Core: 不进入 Core；仅保留 Adapter 边界模式供 UE 侧实现参考。

Strip from Planet_SIM Core: OpenGL rendering、texture manager、geometry manager、render flags、label flags、SDL/Qt/Win frontend。

Planet_SIM target: UE/Cesium/DigitalPlanet/Inspector adapter layer。

Migration level: Boundary pattern only。

Validation: Model / Controller 头文件不反向包含上述 View 资源；View Adapter 可以依赖 Model。
