# Celestia Standard MVC Final Architecture

Date: 2026-06-25

This document records the final MVC architecture reached by Step8 through Step11 in this repository. It supersedes the earlier migration-only wording in the Step5/Step6 documents where those documents describe temporary file-script fallback paths.

## Source Ownership

```mermaid
flowchart TD
    Protocol["src/celruntime/protocol<br/>RuntimeEnvelope, lifecycle, scene.frame, view.input"] --> ModelRuntime["src/celruntime/model<br/>ModelService and SceneExtractor"]
    Protocol --> ControllerRuntime["src/celruntime/controller<br/>ControllerService"]
    Protocol --> ViewRuntime["src/celruntime/view<br/>Debug2D ViewService"]
    Protocol --> View3DRuntime["src/celruntime/view3d<br/>OpenGL3D host runtime"]
    Protocol --> RuntimeProcess["src/celruntime/process<br/>RuntimeSession and host loops"]
    RuntimeProcess --> Transport["src/celruntime/transport<br/>stdio-pipe and local-socket"]
    RuntimeProcess --> Assembly["src/celruntime/assembly<br/>RuntimeAssemblyConfig and runner"]
    RuntimeProcess --> DataPlane["src/celruntime/dataplane<br/>DataPlaneRef and shared-memory channel"]
    View3DRuntime --> View3DImpl["src/celengine/view3d and src/celrender/view3d<br/>existing renderer implementation"]
    Launcher["src/celestia/sdl<br/>launcher and runtime config entry"] --> Assembly
```

## CMake Target Direction

```mermaid
flowchart TD
    Model["celestia_model"] --> ProtocolDTO["protocol DTOs and model-safe contracts"]
    Controller["celestia_controller"] --> Model
    ViewAdapter["celestia_view_adapter"] --> Model
    ViewAdapter --> Controller
    View3D["celestia_view3d"] --> Model
    View3D --> Controller
    View3D --> ViewAdapter
    Runtime["celestia_runtime"] --> RuntimeProtocol["runtime protocol, process, transport, assembly, dataplane"]
    DebugProvider["celestia_viewprovider_debug2d"] --> Runtime
    OpenGLProvider["celestia_viewprovider_3d"] --> Runtime
    OpenGLProvider --> View3D
    AppCore["celestia shared library"] --> Model
    AppCore --> Controller
    AppCore --> ViewAdapter
    AppCore --> View3D
    AppCore --> Runtime
    SDL["celestia-sdl"] --> AppCore
```

Current CMake closure is conservative: historical Celestia in-process rendering still flows through the `celestia` shared library. The new cross-process runtime path is isolated under `celestia_runtime` and no longer depends on the removed Step6 file-script fallback.

## Runtime Process Topology

```mermaid
flowchart LR
    Launcher["SDL launcher or RuntimeAssemblyRunner"] --> ControllerHost["celestia-controller-host"]
    Launcher --> ModelHost["celestia-model-host"]
    Launcher --> ViewHost["celestia-view-host<br/>Debug2D"]
    Launcher --> View3DHost["celestia-view3d-host<br/>OpenGL3D"]
    ControllerHost <--> ModelHost
    ModelHost --> ViewHost
    ModelHost --> View3DHost
    View3DHost --> ControllerHost
```

The launcher selects exactly one active View host at a time. Ordered View switch stops the current View host before starting the replacement View host while keeping Model and Controller alive.

## IPC Message Flow

```mermaid
sequenceDiagram
    participant L as Launcher
    participant C as Controller Host
    participant M as Model Host
    participant V as View Host
    L->>C: runtime.hello
    L->>M: runtime.hello
    L->>V: runtime.hello
    C-->>L: runtime.ready
    M-->>L: runtime.ready
    V-->>L: runtime.ready
    L->>C: runtime.start
    L->>M: runtime.start
    L->>V: runtime.start
    L->>C: controller.tick
    C->>M: model.step
    M->>V: view.frame or scene.frame
    V-->>L: view.frameRendered or view.input
    V-->>C: view.input
    L->>V: runtime.shutdown
    L->>C: runtime.shutdown
    L->>M: runtime.shutdown
```

## View Plugin Lifecycle

```mermaid
flowchart TD
    Discover["discover manifest"] --> Validate["validate ABI and protocol version"]
    Validate --> Load["load plugin or built-in provider"]
    Load --> Start["start View host or View provider"]
    Start --> Active["active rendering or frame consumption"]
    Active --> Stop["ordered stop"]
    Stop --> Unload["unload or replace View"]
    Stop --> Load
```

The current hot-swap guarantee is ordered stop/start. It is not a promise of frame-perfect replacement at an arbitrary point in the render loop.

## Transport And Data Plane

```mermaid
flowchart TD
    RuntimeEnvelope["RuntimeEnvelope control plane"] --> StdioPipe["stdio-pipe transport"]
    RuntimeEnvelope --> LocalSocket["local-socket transport<br/>Windows named pipe"]
    SceneFrame["scene.frame payload"] --> FramedEnvelope["framed-envelope data path"]
    LargeBlock["future large mesh, texture, catalog block"] --> DataPlaneRef["DataPlaneRef"]
    DataPlaneRef --> InProcess["in-process data plane"]
    DataPlaneRef --> SharedMemory["shared-memory data plane"]
```

Step11 preserves the control-plane protocol as the stable boundary. Shared memory exists as a tested data-plane channel, but not every `scene.frame` payload is automatically moved through shared memory yet.

## Interface Ownership

```mermaid
flowchart TD
    IModel["Model public behavior"] --> IProtocol["Protocol messages"]
    IController["Controller public behavior"] --> IProtocol
    IView["View public behavior"] --> IProtocol
    IRuntime["Runtime assembly and transport"] --> IProtocol
    IRuntime --> ITransport["RuntimeTransport"]
    IRuntime --> IDataPlane["DataPlaneChannel"]
    IViewPlugin["View plugin ABI"] --> IView
```

Forbidden directions after Step11:

```text
Protocol -> Renderer/OpenGL/SDL
Protocol -> concrete transport implementation
RuntimeSession -> file-script stdio fallback
SDL launcher -> Step6 stdio-files fallback
Model runtime -> concrete View host internals
View runtime -> concrete Model internals
```

## Final Capability Matrix

| Capability | Status | Evidence |
| --- | --- | --- |
| M / C / V local supervised host processes | Implemented | RuntimeSession, ProcessSupervisor, full CTest |
| 2D Debug View process | Implemented | `runtime-2d-stdio.yaml`, `runtime-2d-local-socket.yaml` |
| 3D OpenGL View process | Implemented | `runtime-3d-stdio.yaml`, `runtime-3d-local-socket.yaml`, Step8/Step10 screenshots |
| Ordered 2D -> 3D View switch | Implemented | `runtime-switch-2d-to-3d-local-socket.yaml` |
| Ordered 3D -> 2D View switch | Implemented | `runtime-switch-3d-to-2d-local-socket.yaml`, Step11 runtime acceptance test |
| View plugin ABI and manifest registry | Implemented | Step9 tests and built-in manifests |
| Runtime assembly config | Implemented | Step10 config loader, examples, RuntimeAssemblyRunner |
| stdio-pipe transport | Implemented | Step7/Step10 tests |
| local-socket transport | Implemented | Windows named-pipe local transport tests and smokes |
| shared-memory data plane | Minimal tested channel | DataPlaneRef and SharedMemoryDataPlane tests |
| Step6 file-script fallback | Removed in Step11 | `scan_mvc_dependencies.ps1` and Step11 tests |
| Root forwarding headers | Removed before Step11, still scanned | `mvc_step5_forwarding_header_removal_test.cpp` |

## Remaining Boundaries

```text
Still true:
  - The existing in-process Celestia renderer remains available through the
    original application shell.
  - Some historical Celestia modules still keep names such as legacy or compat
    because they are product compatibility modules, not MVC migration shims.
  - local-socket means same-machine IPC; it is not TCP or cross-machine runtime.

Not true yet:
  - every render resource is transferred through shared memory by default.
  - arbitrary third-party transport plugins are frozen as a public ecosystem.
  - OpenGL3D cross-process rendering has full historical renderer parity.
```
