# Codex Start Here - Celestia MVC Step 1

## Current Workspace

This directory is the active MVC Step 1 worktree:

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-mvc-step1
```

It is bound to branch:

```text
codex/celestia-mvc-step1
```

The main Celestia checkout is separate:

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

That main checkout is for `master` / upstream-source synchronization. Do not expect the MVC Step 1 code changes to appear there unless this branch is merged or checked out there.

## Remote Model

```text
origin   = https://github.com/wgwtest/Celestia.git
upstream = https://github.com/CelestiaProject/Celestia.git
```

Use `origin` for this project's writable fork. Treat `upstream` as the official Celestia source reference.

The current Step 1 commit pushed to the fork is:

```text
14062ca refactor: decouple Celestia MVC boundaries
```

PR entry:

```text
https://github.com/wgwtest/Celestia/pull/new/codex/celestia-mvc-step1
```

## Current Phase Boundary

Step 1 is code-level MVC decoupling inside Celestia itself. It is not just documentation cleanup.

Step 1 acceptance requires:

1. Celestia source-level MVC boundaries are factually decoupled.
2. A runnable Celestia program is built.
3. Full local unit tests pass.
4. Runtime smoke testing has been performed.
5. Human acceptance is still required before Step 2 migration starts.

Step 2 is migration into another project and must remain independent. Do not start Step 2 core implementation from this worktree unless the user explicitly accepts Step 1 and opens a new Step 2 task.

## First Reading Order

Read these first in a new Codex session:

```text
CODEX_START_HERE.md
DOC\CODEX_DOC\04_研制计划\13-WBS-0.13-Celestia标准MVC解耦-Step1代码落实记录.md
DOC\CODEX_DOC\02_设计说明\02-05-Celestia标准MVC解耦与迁移映射说明.md
```

Then inspect the worktree state:

```powershell
git status --short --branch
git log -3 --oneline --decorate
git remote -v
git worktree list --porcelain
```

## Main Code Changes

Key model/controller boundary reductions:

```text
src/celengine/simulation.h/.cpp
src/celengine/universe.h/.cpp
src/celengine/body.h/.cpp
src/celengine/star.h/.cpp
src/celengine/deepskyobj.h/.cpp
```

New View Adapter / boundary helper files:

```text
src/celengine/selectionpicker.h/.cpp
src/celengine/bodyrenderassets.h/.cpp
src/celengine/starrenderassets.h/.cpp
src/celengine/nebularenderassets.h/.cpp
src/celengine/bodylocationgeometryprojector.h/.cpp
src/celengine/deepskyobjectrenderpolicy.h/.cpp
src/celengine/deepskyobjectpicker.h/.cpp
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
test/unit/CMakeLists.txt
```

## Verification Commands

This machine does not expose `cmake` / `ctest` in the normal PowerShell PATH. Use Visual Studio's bundled CMake tools directly:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build-mvc-sdl-rel --config Release
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel --output-on-failure

& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build-mvc-baseline-rel --config Release
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-baseline-rel --output-on-failure
```

Last verified result before this handoff:

```text
build-mvc-sdl-rel: ninja: no work to do
build-mvc-sdl-rel: 42/42 tests passed
build-mvc-baseline-rel: ninja: no work to do
build-mvc-baseline-rel: 42/42 tests passed
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

If a font-loading warning appears in the console, it does not by itself invalidate the MVC boundary work. It affects text visibility in that local runtime setup.

## Git Hygiene

Do not commit generated build/runtime assets:

```text
build-*
run-*
CelestiaContent copies inside build output
downloaded runtime catalogs
local screenshots or temporary demo data
```

Before committing, run:

```powershell
git status --short --branch
git diff --check
```

Push Step 1 work only to the fork:

```powershell
git push origin codex/celestia-mvc-step1
```

Do not push to `upstream`.

## Upstream Sync Warning

This Step 1 branch was created from Celestia commit:

```text
44ec265 Move InfoURL into a separate manager class
```

The fork and upstream `master` later advanced beyond that point. Some upstream changes touch files that overlap the MVC refactor, including `render.cpp`, `renderglsl.cpp`, `solarsys.cpp`, `stardbbuilder.cpp`, `universe.cpp/.h`, and `celestiacore.cpp`.

If rebasing or merging this branch onto a newer `upstream/master`, treat that as a separate integration task and rerun the full build/test/runtime checks.

Also note: recursive fetch may hit an upstream submodule missing-ref issue in `src/tools/celestia-gaia-stardb`. Fetching branch refs with `--no-recurse-submodules` worked during this handoff.
