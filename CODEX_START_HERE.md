# Codex Start Here - Celestia MVC Step 3

## Current Workspace

This directory is the active MVC Step 3 worktree:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step1
```

The directory name was created during Step 1, but the active branch is now:

```text
codex/celestia-mvc-step3
```

The main Celestia checkout is separate:

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

That main checkout is for `master` / upstream-source synchronization. Do not expect the MVC Step 3 code changes to appear there unless this branch is merged or checked out there.

## Remote Model

```text
origin   = https://github.com/wgwtest/Celestia.git
upstream = https://github.com/CelestiaProject/Celestia.git
```

Use `origin` for this project's writable fork. Treat `upstream` as the official Celestia source reference.

Step 1 was pushed to the fork as:

```text
14062ca refactor: decouple Celestia MVC boundaries
```

Step 3 is on:

```text
codex/celestia-mvc-step3
```

PR entry after pushing:

```text
https://github.com/wgwtest/Celestia/pull/new/codex/celestia-mvc-step3
```

## Current Phase Boundary

Step 1 is complete: Celestia source-level MVC boundaries were reduced so Model / Controller public headers no longer expose the main View resources.

Step 2 is complete in this worktree: Celestia Model implementation files no longer call concrete render-asset sidecars for Body, StarDetails, and Nebula; DSO semantic loading is split from Nebula mesh asset loading; SelectionPicker consumes a replaceable geometry provider; and `SceneViewModel` provides a headless View Adapter proof.

Step 3 is complete in this worktree: CMake now exposes auditable `celestia_model`, `celestia_controller`, `celestia_view_adapter`, and `celestia_view3d` object targets; `celestia` explicitly aggregates them through the application shell; Step3 contract tests scan CMake ownership buckets, target dependency direction, and Model / Controller implementation files.

Step 3 is still an in-process physical modularization step. It is not an OS-process split for M / V / C, and it is not the Planet_SIM clean-room migration. Directory `git mv` reorganization is intentionally deferred to a possible Step3.1 after the target split remains stable.

Planet_SIM clean-room migration is a later independent migration phase. It may consume the Step 1 / Step 2 / Step 3 boundary evidence, but should not be started from this worktree unless the user explicitly opens that migration task.

## First Reading Order

Read these first in a new Codex session:

```text
CODEX_START_HERE.md
DOC\CODEX_DOC\04_研制计划\13-WBS-0.13-Celestia标准MVC解耦-Step1代码落实记录.md
DOC\CODEX_DOC\04_研制计划\14-WBS-0.14-Celestia标准MVC解耦-Step2落地方案.md
DOC\CODEX_DOC\04_研制计划\15-WBS-0.15-Celestia标准MVC解耦-Step3物理模块化与构建边界固化方案.md
DOC\CODEX_DOC\02_设计说明\02-05-Celestia标准MVC解耦与迁移映射说明.md
```

Then inspect the worktree state:

```powershell
git status --short --branch
git log -8 --oneline --decorate
git remote -v
git worktree list --porcelain
```

## Main Code Changes

Step 1 model/controller boundary reductions:

```text
src/celengine/simulation.h/.cpp
src/celengine/universe.h/.cpp
src/celengine/body.h/.cpp
src/celengine/star.h/.cpp
src/celengine/deepskyobj.h/.cpp
```

Step 1 View Adapter / boundary helper files:

```text
src/celengine/selectionpicker.h/.cpp
src/celengine/bodyrenderassets.h/.cpp
src/celengine/starrenderassets.h/.cpp
src/celengine/nebularenderassets.h/.cpp
src/celengine/bodylocationgeometryprojector.h/.cpp
src/celengine/deepskyobjectrenderpolicy.h/.cpp
src/celengine/deepskyobjectpicker.h/.cpp
```

Step 2 lifecycle, provider, asset-loader, and headless View Adapter files:

```text
src/celengine/bodylifecycle.h/.cpp
src/celengine/stardetailslifecycle.h/.cpp
src/celengine/nebulalifecycle.h/.cpp
src/celengine/nebularenderassetloader.h/.cpp
src/celengine/selectiongeometryprovider.h
src/celengine/sceneviewmodel.h/.cpp
```

Step 3 physical CMake boundary files:

```text
src/celengine/CMakeLists.txt
src/celestia/CMakeLists.txt
test/unit/mvc_step3_contract_test.cpp
```

Application shell and frontend call-site changes:

```text
src/celestia/celestiacore.cpp
src/celestia/qt/qtselectionpopup.cpp
src/celestia/win32/wincontextmenu.cpp
src/celestia/win32/winmainwindow.cpp
```

Boundary regression tests:

```text
test/unit/mvc_boundary_test.cpp
test/unit/mvc_step2_contract_test.cpp
test/unit/mvc_step3_contract_test.cpp
test/unit/CMakeLists.txt
```

## Verification Commands

This machine does not expose `cmake` / `ctest` in the normal PowerShell PATH. Use Visual Studio's bundled CMake tools directly:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build-mvc-baseline-rel --config Release
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-baseline-rel --output-on-failure

& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build-mvc-sdl-rel --config Release
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel --output-on-failure
```

Last verified Step 3 result before this handoff:

```text
build-mvc-baseline-rel: build passed
build-mvc-baseline-rel: 53/53 tests passed
build-mvc-sdl-rel: build passed
build-mvc-sdl-rel: 53/53 tests passed
SDL smoke: build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe entered the run loop for 6 seconds with build-mvc-sdl-rel\run-full and was stopped
```

## Runtime Notes

The runnable SDL executable is:

```text
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe
```

Runtime demo/catalog directories under build output are local verification assets and should not be committed.

Known local runtime directories used during verification included:

```text
build-mvc-sdl-rel\run-minimal
build-mvc-sdl-rel\run-full
build-mvc-sdl-rel\run-full-nofont
build-mvc-sdl-rel\run-mass-orbits
```

Before a full SDL visual smoke run, refresh `run-full` through the CMake install
rules so it includes the complete core data set:

```powershell
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --install build-mvc-sdl-rel --prefix (Resolve-Path 'build-mvc-sdl-rel\run-full').Path --component core
```

`build-mvc-sdl-rel\run-full\shaders\text_vert.glsl` and
`build-mvc-sdl-rel\run-full\shaders\text_frag.glsl` must exist before launch.
If the SDL app starts with readable menu text but core HUD/label text appears
as solid red blocks, the `shaders` directory is missing from the active data
directory and the text shader is falling back to Celestia's red error shader.

If a font-loading warning appears in the console, it does not by itself invalidate the MVC boundary work. It affects text visibility in that local runtime setup.

## Git Hygiene

Do not commit generated build/runtime assets:

```text
build-*
run-*
CelestiaContent copies inside build output
downloaded runtime catalogs
local screenshots or temporary demo data
verify-*.log
shaders.log
```

Before committing, run:

```powershell
git status --short --branch
git diff --check
```

Push Step 3 work only to the fork:

```powershell
git push origin codex/celestia-mvc-step3
```

Do not push to `upstream`.

## Upstream Sync Warning

This Step 3 branch descends from the Step 1 / Step 2 work, which was created from Celestia commit:

```text
44ec265 Move InfoURL into a separate manager class
```

The fork and upstream `master` later advanced beyond that point. Some upstream changes touch files that overlap the MVC refactor, including `render.cpp`, `renderglsl.cpp`, `solarsys.cpp`, `stardbbuilder.cpp`, `universe.cpp/.h`, and `celestiacore.cpp`.

If rebasing or merging this branch onto a newer `upstream/master`, treat that as a separate integration task and rerun the full build/test/runtime checks.

Also note: recursive fetch may hit an upstream submodule missing-ref issue in `src/tools/celestia-gaia-stardb`. Fetching branch refs with `--no-recurse-submodules` worked during this handoff.
