# Codex Start Here - Celestia MVC Current Baseline

## Current Workspace

Use this directory as the canonical working directory for new Codex sessions:

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

The active branch is:

```text
codex/celestia-mvc-step12-scene-frame
```

The latest pushed master is:

```text
d6b049c docs: plan Celestia MVC real scene projection
origin/master = d6b049c
```

The only remaining auxiliary worktree is the fixed pre-MVC compatibility baseline:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-compat-baseline-44ec265
```

Do not remove it unless the compatibility regression baseline is intentionally regenerated or relocated.

## Remote Model

```text
origin   = https://github.com/wgwtest/Celestia.git
upstream = https://github.com/CelestiaProject/Celestia.git
```

Use `origin` as the writable fork. Treat `upstream` as the official Celestia reference only. Do not push to `upstream`.

## Current Architecture State

The repository now has three active lines:

```text
1. Ordinary SDL unified exe / in-process historical Celestia path.
2. Source and CMake level MVC boundaries under src/celengine.
3. Local multi-process MVC runtime under src/celruntime.
```

Implemented and merged:

```text
Step1  source-level MVC boundary reduction
Step2  deeper Model/ViewAdapter decoupling
Step3  CMake target boundary split
Step4  physical MVC directory reorganization
Step5  runtime decoupling and view-provider baseline
Step6  runtime protocol and host process smoke
Step7  long-running IPC and process supervision
Step8  cross-process OpenGL3D View host and scene.frame protocol
Step9  View plugin ABI, manifests, registry, and ordered switching
Step10 runtime assembly config, transport abstraction, local-socket, data-plane
Step11 compatibility fallback removal and final architecture closure
Compatibility regression harness for original unified exe capability checks
```

Active development branch:

```text
Step12 scene.frame vNext contract and Model Lock Policy
branch: codex/celestia-mvc-step12-scene-frame
```

Current consolidated status and compatibility conclusion:

```text
DOC\CODEX_DOC\02_设计说明\02-09-Celestia真实场景投影与SceneFrame契约.md
DOC\CODEX_DOC\02_设计说明\02-08-Celestia当前解耦现状与MVC能力盘点.md
DOC\CODEX_DOC\06_测试文档\04_结论报告\01-Celestia统一exe原能力保真阶段结论.md
```

## Runtime Capabilities

The current runtime can launch separate local host processes:

```text
celestia-model-host
celestia-controller-host
celestia-view-host
celestia-view3d-host
```

Supported runtime transport paths:

```text
stdio-pipe
local-socket
```

Supported View paths in the current runtime baseline:

```text
celestia.view2d.debug
celestia.view3d.opengl
```

Important boundary: the cross-process `celestia-view3d-host` is currently a minimal protocol-validation renderer. It proves independent View3D process startup, `scene.frame` delivery, `view.input` return flow, and process supervision. It is not yet visually equivalent to the historical in-process Celestia renderer.

## Current User-Facing Interpretation

It is correct to say:

```text
Celestia now has a local, multi-process, runtime-configurable MVC baseline.
M / C / V host processes can be started, supervised, messaged, switched, and shut down.
Debug2D and OpenGL3D are available through the same runtime assembly path.
The ordinary SDL unified exe / in-process path is still the main original-capability path.
```

It is not correct to say yet:

```text
The cross-process View3D has full historical Celestia visual parity.
The Model exports the complete Celestia Universe/StarDatabase/Body/Orbit state across process boundaries.
All texture, mesh, star catalog, and UI resources use the final data-plane contract.
Arbitrary third-party View plugins are a frozen public ecosystem.
This is a cross-machine distributed three-service architecture.
Qt and Win32 frontend capability parity has been validated by the SDL regression harness.
```

## Likely Next Work

Suggested next phase name:

```text
Step12 - Real Celestia scene projection and View3D visual parity path
```

Formal execution plan:

```text
DOC\CODEX_DOC\04_研制计划\28-WBS-0.28-Celestia标准MVC解耦-Step12-18真实场景投影与View3D保真执行计划.md
```

Detailed active Step12 plan:

```text
DOC\CODEX_DOC\04_研制计划\29-WBS-0.29-Celestia标准MVC解耦-Step12真实SceneFrame契约方案.md
```

Step12 focuses on:

```text
1. Define scene.frame vNext protocolVersion and DTO fields.
2. Add TimeState, ResourceRef vNext, structured LabelRenderState, and minimal DSO fields.
3. Add validateSceneFrame / isValidSceneFrame.
4. Add valid and invalid .sceneframe fixtures.
5. Keep Step8 legacy scene.frame deserialization compatible.
6. Keep historical in-process rendering available as a regression reference.
```

After Step12 acceptance, Step13 should start the headless real Model Backend work. Do not start Step13 before Step12 verification evidence is recorded under:

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\
```

## First Reading Order

For a new session, read in this order:

```text
CODEX_START_HERE.md
DOC\CODEX_DOC\04_研制计划\29-WBS-0.29-Celestia标准MVC解耦-Step12真实SceneFrame契约方案.md
DOC\CODEX_DOC\04_研制计划\28-WBS-0.28-Celestia标准MVC解耦-Step12-18真实场景投影与View3D保真执行计划.md
DOC\CODEX_DOC\02_设计说明\02-09-Celestia真实场景投影与SceneFrame契约.md
DOC\CODEX_DOC\02_设计说明\02-08-Celestia当前解耦现状与MVC能力盘点.md
DOC\CODEX_DOC\06_测试文档\04_结论报告\01-Celestia统一exe原能力保真阶段结论.md
DOC\CODEX_DOC\02_设计说明\02-07-Celestia标准MVC最终架构说明.md
DOC\CODEX_DOC\02_设计说明\02-06-Celestia-MVC-Runtime-Protocol-v1.md
DOC\CODEX_DOC\04_研制计划\27-WBS-0.27-Celestia标准MVC解耦-Step11实现证据.md
DOC\CODEX_DOC\04_研制计划\26-WBS-0.26-Celestia标准MVC解耦-Step10实现证据.md
DOC\CODEX_DOC\04_研制计划\25-WBS-0.25-Celestia标准MVC解耦-Step9实现证据.md
DOC\CODEX_DOC\04_研制计划\24-WBS-0.24-Celestia标准MVC解耦-Step8实现证据.md
```

Then inspect current state:

```powershell
git status --short --branch
git log -8 --oneline --decorate
git remote -v
git worktree list --porcelain
```

## Main Runtime Areas

Runtime protocol, host process, transport, assembly, and data-plane:

```text
src/celruntime/protocol/
src/celruntime/process/
src/celruntime/transport/
src/celruntime/assembly/
src/celruntime/dataplane/
src/celruntime/model/
src/celruntime/controller/
src/celruntime/view/
src/celruntime/view3d/
src/celruntime/viewplugin/
```

Current source-level MVC buckets:

```text
src/celengine/model/
src/celengine/controller/
src/celengine/adapter/
src/celengine/view3d/
src/celengine/legacy/
```

Existing in-process Celestia renderer and 3D implementation areas:

```text
src/celengine/view3d/
src/celrender/view3d/
src/celrender/view3d/gl/
```

Runtime config examples:

```text
DOC\CODEX_DOC\examples\runtime-2d-stdio.yaml
DOC\CODEX_DOC\examples\runtime-2d-local-socket.yaml
DOC\CODEX_DOC\examples\runtime-3d-stdio.yaml
DOC\CODEX_DOC\examples\runtime-3d-local-socket.yaml
DOC\CODEX_DOC\examples\runtime-switch-2d-to-3d-local-socket.yaml
DOC\CODEX_DOC\examples\runtime-switch-3d-to-2d-local-socket.yaml
```

## Verification Commands

This machine normally needs Visual Studio's CMake and CTest paths:

```powershell
$vsdev = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat'
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
```

MVC boundary scans:

```powershell
powershell -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
```

Original capability compatibility regression harness:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

Step12 focused verification:

```powershell
cmd /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul && "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build-mvc-sdl-rel --config Release --target unit -- -k 0'
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step8|Step12|SceneFrame|SceneProtocol" --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
```

The fixed pre-MVC comparison baseline is:

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

The first harness version covers SDL unified exe / in-process screenshots. It does not claim Qt or Win32 frontend parity.

Last verified after merging into `master`:

```text
compat Full:            2026-06-27-155813, c6d7e53, pass
scan_mvc_dependencies:  passed during Full
scan_cmake_targets:     passed during Full
runtime smoke:          6/6 passed during Full
visual scenes:          8/8 passed during Full
```

Latest machine report:

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\2026-06-27-155813-Celestia-compat-regression-machine-report.md
```

## Startup Commands

Run current cross-process OpenGL3D demo:

```powershell
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe `
  --dir build-mvc-sdl-rel `
  --mvc-mode=multi-process `
  --view=celestia.view3d.opengl `
  --serve `
  --duration-ms=600000 `
  --host-transport=local-socket
```

Run Debug2D demo:

```powershell
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe `
  --runtime-config DOC\CODEX_DOC\examples\runtime-2d-local-socket.yaml
```

Run View switch demo:

```powershell
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe `
  --runtime-config DOC\CODEX_DOC\examples\runtime-switch-2d-to-3d-local-socket.yaml
```

Before launching demos, confirm there are no stale runtime processes:

```powershell
Get-Process -ErrorAction SilentlyContinue |
  Where-Object { $_.ProcessName -in @('celestia-sdl','celestia-model-host','celestia-controller-host','celestia-view-host','celestia-view3d-host') }
```

## Git Hygiene

Generated build/runtime artifacts should not be committed:

```text
build-*
run-*
local screenshots
temporary trace/log files
downloaded runtime catalogs
shaders.log
.regression-artifacts
```

Before committing:

```powershell
git status --short --branch
git diff --check
```

Push normal completed work to the fork:

```powershell
git push origin master
```

Do not push to `upstream`.

## Worktree Cleanup Policy

Only the fixed baseline worktree remains:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-compat-baseline-44ec265
```

It is retained for the original-capability compatibility regression baseline. Historical MVC worktrees and the compatibility-regression implementation worktree have already been removed after their branches were merged into `master`.
