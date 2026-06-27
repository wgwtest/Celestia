# Codex Start Here - Celestia MVC Current Baseline

## Current Workspace

Use this directory as the canonical working directory for new Codex sessions:

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

The active branch in the canonical checkout is:

```text
master
```

The compatibility-regression implementation worktree is:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-compat-regression
```

Its active branch is:

```text
codex/celestia-compat-regression
```

The latest MVC result has already been merged into `master` and pushed to the user's fork:

```text
57cb296 refactor: close MVC Step11 compatibility layer
origin/master = 57cb296
```

The old worktrees are no longer the primary working locations:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step1
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step8-11
```

They are clean and their branch histories are already contained in `master`. Keep them only for historical inspection unless the user explicitly asks to remove them.

## Remote Model

```text
origin   = https://github.com/wgwtest/Celestia.git
upstream = https://github.com/CelestiaProject/Celestia.git
```

Use `origin` as the writable fork. Treat `upstream` as the official Celestia reference only. Do not push to `upstream`.

## Current Architecture State

The repository now has a verified local multi-process MVC runtime baseline.

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
```

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
```

It is not correct to say yet:

```text
The cross-process View3D has full historical Celestia visual parity.
The Model exports the complete Celestia Universe/StarDatabase/Body/Orbit state across process boundaries.
All texture, mesh, star catalog, and UI resources use the final data-plane contract.
Arbitrary third-party View plugins are a frozen public ecosystem.
This is a cross-machine distributed three-service architecture.
```

## Likely Next Work

If the user asks why the new View3D looks simpler than before, the answer is:

```text
The current cross-process View3D uses a synthetic SceneFrame and placeholder rendering.
The historical in-process renderer still has the full visual pipeline.
The next hard stage is real Celestia scene projection and renderer/resource integration.
```

Suggested next phase name:

```text
Step12 - Real Celestia scene projection and View3D visual parity path
```

Step12 should focus on:

```text
1. Map real Celestia Universe / Simulation / Observer / Body / Star data into scene.frame.
2. Replace placeholder SceneExtractor output with real read-only scene projection.
3. Add ResourceRef/data-plane coverage for real catalogs, textures, meshes, and orbit assets.
4. Reuse or adapt the historical renderer inside celestia-view3d-host without breaking process boundaries.
5. Extend view.input into real navigation, selection, camera, and interaction commands.
6. Keep historical in-process rendering available as a regression reference.
```

Do not start Step12 implementation without first writing or updating its plan under `DOC\CODEX_DOC\04_研制计划\`.

## First Reading Order

For a new session, read in this order:

```text
CODEX_START_HERE.md
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

Baseline build and full test:

```powershell
cmd.exe /c "call `"$vsdev`" -arch=x64 -host_arch=x64 >NUL && `"$cmake`" --build build-mvc-baseline-rel --config Release --target unit && `"$ctest`" --test-dir build-mvc-baseline-rel -C Release --output-on-failure"
```

SDL build and full test:

```powershell
cmd.exe /c "call `"$vsdev`" -arch=x64 -host_arch=x64 >NUL && `"$cmake`" --build build-mvc-sdl-rel --config Release --target unit celestia-sdl && `"$ctest`" --test-dir build-mvc-sdl-rel -C Release --output-on-failure"
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

The fixed pre-MVC comparison baseline is:

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

The first harness version covers SDL unified exe / in-process screenshots. It does not claim Qt or Win32 frontend parity.

Last verified after merging into `master`:

```text
build-mvc-baseline-rel: 157/157 tests passed
build-mvc-sdl-rel:      157/157 tests passed
scan_mvc_dependencies:  passed
scan_cmake_targets:     passed
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

Existing historical worktrees are currently clean and fully contained in `master`, but do not remove them without explicit user confirmation.

If the user confirms cleanup, remove from the main repo root, not from inside the worktree:

```powershell
git worktree remove D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step8-11
git worktree remove D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step1
git worktree prune
```

Only delete local branches after confirming the user no longer wants them for history navigation.
