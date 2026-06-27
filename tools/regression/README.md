# Celestia Compatibility Regression Harness

This directory contains the regression harness for checking that the ordinary SDL unified exe / in-process path still covers the major visible capabilities of the fixed pre-MVC baseline.

## Commands

Fast script and helper validation, no build and no Celestia launch:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
```

Current checkout gate:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

Create or refresh the fixed pre-MVC screenshot baseline:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
```

Full baseline/current comparison:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

## Useful Options

```powershell
-SkipBuild
-SkipRuntimeSmoke
-CurrentBuildDir D:\path\to\build-mvc-sdl-rel
-ArtifactsRoot D:\path\to\.regression-artifacts\Celestia
-BaselineWorktree D:\path\to\celestia-compat-baseline-44ec265
-KeepTemp
```

`-SkipBuild` is useful when a compatible `build-mvc-sdl-rel` already exists. The build directory must still contain `src\celestia\sdl\celestia-sdl.exe` and a runtime content root, normally `run-full\celestia.cfg`.

## Artifact Layout

Generated builds, screenshots, metrics, logs, and contact sheets stay outside the repository by default:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\
  baselines\44ec265\
  runs\<timestamp>-<commit>-<mode>\
```

`Full` also copies a Markdown machine report into:

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\
```

## Scope

This harness intentionally focuses on SDL unified exe / in-process visual compatibility. It does not prove exhaustive Celestia feature parity, pixel-perfect rendering, or Qt/Win32 frontend parity.
