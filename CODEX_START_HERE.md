# Codex Start Here - Celestia MVC Current Baseline

## Current Workspace

Use this directory as the canonical working directory for new Codex sessions:

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

The active branch is:

```text
codex/celestia-mvc-step13-real-model-backend
```

The latest pushed master is:

```text
9983a96 docs: strengthen SceneFrame protocol specification
origin/master = 9983a96
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
Step14 real SceneExtractor projection
branch: codex/celestia-mvc-step13-real-model-backend
```

Current consolidated status and compatibility conclusion:

```text
DOC\CODEX_DOC\03_协议规范\03-01-Celestia-SceneFrame-vNext协议规范.md
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

Important boundary: the cross-process `celestia-view3d-host` now consumes real `scene.frame` body/star/orbit/resource fields and reports those counts through `view.frameRendered`. Its OpenGL output is still a simplified protocol visualization, not historical in-process Celestia renderer parity.

## Current User-Facing Interpretation

It is correct to say:

```text
Celestia now has a local, multi-process, runtime-configurable MVC baseline.
M / C / V host processes can be started, supervised, messaged, switched, and shut down.
The Model host can be configured with a real Celestia data root and load real Universe / Simulation state headlessly.
The Model host can now project a first real scene.frame with real time, observer/camera, selected body/star, orbit sample, and catalog resources.
The View3D host can now consume real scene.frame body/star/orbit/resource fields and resolve ResourceRef values through a content root.
The first Step16 Controller loops are implemented: View3D Space pauses the Model, View3D L changes time scale, View3D MouseWheel changes scene camera FOV, View3D Ctrl+Backspace clears scene selection, View3D H selects Sol, View3D C centers the camera output on the current selection, View3D Left orbits the camera yaw output, View3D G navigates the camera output to the current selection, and View3D F follows the current selection in observer output state; the next scene.frame reports those output changes.
Debug2D and OpenGL3D are available through the same runtime assembly path.
The ordinary SDL unified exe / in-process path is still the main original-capability path.
```

It is not correct to say yet:

```text
The cross-process View3D has full historical Celestia visual parity.
The Model exports the complete Celestia Universe/StarDatabase/Body/Orbit state across process boundaries.
All texture, mesh, star catalog, and UI resources use the final data-plane protocol.
Arbitrary third-party View plugins are a frozen public ecosystem.
This is a cross-machine distributed three-service architecture.
Qt and Win32 frontend capability parity has been validated by the SDL regression harness.
```

## Likely Next Work

Suggested next phase name:

```text
Step17 - ResourceRef and DataPlane stabilization
```

Formal execution plan:

```text
DOC\CODEX_DOC\04_研制计划\28-WBS-0.28-Celestia标准MVC解耦-Step12-18真实场景投影与View3D保真执行计划.md
```

Detailed next Step17 plan:

```text
DOC\CODEX_DOC\04_研制计划\34-WBS-0.34-Celestia标准MVC解耦-Step17资源引用与DataPlane方案.md
```

Current Step17 implementation status:

```text
Task 1 complete locally:
- ResourceRef resolver reports Resolved, MissingRequired, and Invalid status.
- ResourceRef resolver generates stable cache keys by dataPlaneKey, contentHash, then package|kind|relativePath.
- ResourceRef resolver rejects absolute paths and traversal paths.

Task 2 complete locally:
- View3DSceneState reports invalidResourceCount.
- View3DHost frameRendered payload reports invalidResourceCount and dataPlaneEligibleResourceCount.
- View3DHost emits view.resourceMissing events for required missing and invalid resources.

Task 3 complete locally:
- ResourceRef.dataPlaneKey can carry a serialized DataPlaneRef.
- View3D resource resolver parses DataPlaneRef values.
- DataPlane-eligible resources are counted when kind is eligible and dataPlaneKey is parseable.
- Legacy arbitrary dataPlaneKey values remain cache keys without being counted as DataPlane-eligible.

Task 4 complete locally:
- Real Model resource ids and resolver cache keys are stable across sampled frames.
- No production-code stabilization was required for RealModelBackend or SceneExtractor.

Latest focused verification:
- Step17 ResourceRef resolver: 1/1 passed.
- Step17 View3DHost missing/invalid resources: 1/1 passed.
- Step17 DataPlane bridge: 3/3 passed.
- Step17 real Model resource stability guard: 1/1 passed.
- Step15 View3D plus Step17 spot check: 8/8 passed.
- Step12-17 protocol/runtime broad check: 50/50 passed.
- MVC dependency scan passed.
- MVC CMake target scan passed.
- Forbidden terminology scan passed.
- git diff --check reported CRLF warnings only.

Continue with Step17 Task 5:
- Run final Step17 focused verification.
- Run broad Step12-17 verification.
- Build celestia-sdl.
- Run Quick screenshot/runtime regression.
```

Detailed completed Step13/Step14/Step15/Step16 interaction-loop plans:

```text
DOC\CODEX_DOC\04_研制计划\33-WBS-0.33-Celestia标准MVC解耦-Step16真实交互闭环方案.md
DOC\CODEX_DOC\04_研制计划\32-WBS-0.32-Celestia标准MVC解耦-Step15真实View3D消费方案.md
DOC\CODEX_DOC\04_研制计划\30-WBS-0.30-Celestia标准MVC解耦-Step13真实ModelBackend方案.md
DOC\CODEX_DOC\04_研制计划\31-WBS-0.31-Celestia标准MVC解耦-Step14真实SceneExtractor方案.md
DOC\CODEX_DOC\04_研制计划\29-WBS-0.29-Celestia标准MVC解耦-Step12真实SceneFrame协议方案.md
```

Step16 interaction-loop completion means:

```text
1. RealModelBackend still loads celestia.cfg, stars, DSO, SSO, Universe, Simulation, and ObserverSettings headlessly.
2. SceneViewModel now exposes real time, observer/camera, selected body/star, sampled orbit, and catalog resource fields in ViewFrame.
3. viewframecodec preserves those fields across the independent Model host payload path.
4. SceneExtractor projects those fields into placeholder-free scene.frame output for the real backend.
5. View3D host consumes real scene.frame fields and reports body/star/orbit/resource counts through view.frameRendered.
6. Runtime assembly and SDL --serve pass the real content root to View3D so ResourceRef values can be resolved.
7. View3D Space key view.input now reaches ControllerService, becomes model.setPaused, mutates Model paused state, returns scene.frame time.paused=true, and is acknowledged by View3D.
8. View3D L key view.input now reaches ControllerService, becomes model.setTimeScale, mutates Model timeScale state, returns scene.frame time.timeScale=2, and is acknowledged by View3D.
9. View3D MouseWheel view.input now reaches ControllerService, becomes model.setCameraFov, mutates Model camera FOV output state, returns scene.frame camera.fov=40, and is acknowledged by View3D.
10. View3D Ctrl+Backspace view.input now reaches ControllerService, becomes model.clearSelection, clears Model selection output state, returns empty scene.frame selection.type/id, and is acknowledged by View3D.
11. View3D H key view.input now reaches ControllerService, becomes model.setSelection, mutates Model selection output state, returns scene.frame selection.type=star selection.id=celestia:star:Sol, and is acknowledged by View3D.
12. View3D C key view.input now reaches ControllerService, becomes model.centerSelection, mutates Model camera output state, returns scene.frame camera/observer position z=4, and is acknowledged by View3D.
13. View3D Left key view.input now reaches ControllerService, becomes model.orbitCamera, mutates Model camera orientation output state, returns changed scene.frame camera.orientation, and is acknowledged by View3D.
14. View3D G key view.input now reaches ControllerService, becomes model.gotoObject, mutates Model navigation output state, returns scene.frame camera/observer position z=2, and is acknowledged by View3D.
15. View3D F key view.input now reaches ControllerService, becomes model.followObject, mutates Model observer follow output state, returns scene.frame observer.referenceBodyId=celestia:star:Sol observer.frame=celestia:observer:follow, and is acknowledged by View3D.
16. The current Step16 typed-output command list is complete; historical interaction semantics and visual fidelity remain later work.
```

After the current Step16 interaction-loop acceptance, continue with Step17/18 or visual-fidelity work. Do not claim View3D historical renderer parity until later visual-fidelity work has direct screenshot evidence.

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\
```

## First Reading Order

For a new session, read in this order:

```text
CODEX_START_HERE.md
DOC\CODEX_DOC\04_研制计划\33-WBS-0.33-Celestia标准MVC解耦-Step16真实交互闭环方案.md
DOC\CODEX_DOC\04_研制计划\32-WBS-0.32-Celestia标准MVC解耦-Step15真实View3D消费方案.md
DOC\CODEX_DOC\04_研制计划\31-WBS-0.31-Celestia标准MVC解耦-Step14真实SceneExtractor方案.md
DOC\CODEX_DOC\04_研制计划\30-WBS-0.30-Celestia标准MVC解耦-Step13真实ModelBackend方案.md
DOC\CODEX_DOC\04_研制计划\29-WBS-0.29-Celestia标准MVC解耦-Step12真实SceneFrame协议方案.md
DOC\CODEX_DOC\04_研制计划\28-WBS-0.28-Celestia标准MVC解耦-Step12-18真实场景投影与View3D保真执行计划.md
DOC\CODEX_DOC\03_协议规范\03-01-Celestia-SceneFrame-vNext协议规范.md
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

Step13 focused verification:

```powershell
cmd.exe /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >NUL && "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "D:\WorkSpace\Codex\CeleNew\Celestia\build-mvc-sdl-rel" --config Release --target unit -- -k 0'
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step13|RealModelBackend|Headless" --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step8|Step10|Step12|SceneFrame|SceneProtocol|RuntimeAssembly" --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

Step14 focused verification:

```powershell
cmd.exe /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >NUL && "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "D:\WorkSpace\Codex\CeleNew\Celestia\build-mvc-sdl-rel" --config Release --target unit -- -k 0'
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step14|RealSceneExtractor|scene.frame" --output-on-failure
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step8|Step10|Step12|Step13|Step14|SceneFrame|SceneProtocol|RuntimeAssembly" --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

The fixed pre-MVC comparison baseline is:

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

The first harness version covers SDL unified exe / in-process screenshots. It does not claim Qt or Win32 frontend parity.

Last verified on Step12 branch:

```text
commit:                 0fd93ad Step12 scene.frame protocol implementation
protocol spec:          Step12.1 enhanced with constraint levels, existing capability mapping, and enhancement gates
compat Full:            2026-06-27-173305, pass
scan_mvc_dependencies:  passed during Full
scan_cmake_targets:     passed during Full
runtime smoke:          6/6 passed during Full
visual scenes:          8/8 passed during Full
```

Latest machine report:

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\2026-06-27-173305-Celestia-compat-regression-machine-report.md
```

Latest Step13 Quick regression report:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-27-191539-9983a96-quick\machine-report.md
```

Latest Step14 Quick regression report:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-27-194352-fb6609c-quick\machine-report.md
```

Latest Step15 Quick regression report:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-27-201808-742cf34-quick\machine-report.md
```

Latest Step15 View3D runtime screenshot evidence:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\step15-view3d-runtime-20260627-200623\view3d-window.png
```

Latest Step16 interaction-loop Quick regression report:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-085326-d4d0114-quick\machine-report.md
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
