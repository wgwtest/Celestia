# Celestia Compatibility Regression Harness

This harness verifies the covered unified SDL executable behavior against the fixed pre-MVC baseline and runs the current multi-process Runtime checkpoints. Scenario identity and checkpoints come from `verification-matrix.json`.

## Commands

Run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\helpers\Run-CelestiaCompatFaultInjection.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Step18
```

PowerShell callers must inspect `$LASTEXITCODE`:

| Status | Exit code | Meaning |
|---|---:|---|
| `pass` | 0 | All executed required checks passed |
| `fail` | 1 | A target, artifact, or known verification check failed |
| `warn` | 2 | Manual review is required; this is not an automatic pass |
| `error` | 3 | The harness, matrix, dependency, or environment is unusable |

The existence of `machine-report.json` or `machine-report.md` does not mean the run passed.

## Baseline

The baseline commit is `44ec265659d2aa666cbf7546e36e4dde471d54ba`. `InitBaseline` is the only mode allowed to create or replace baseline images and `baseline-manifest.json`.

`Full` validates the manifest commit, exact scenario/checkpoint/image set, SHA-256 values, disk PNG set, and image health. An invalid baseline causes `Full` to return 1 with an explicit `InitBaseline` instruction; `Full` never repairs the baseline automatically.

## Current Matrix

The unified executable group currently contains ten `.cel` scenarios, each with one final image checkpoint. A current `Full` therefore creates ten baseline/current comparisons and one contact sheet. Do not hard-code this count in new logic; the JSON matrix remains the source.

The Runtime group contains six scenarios. It verifies process exit, trace creation, clean shutdown, transport-specific lifecycle, positive 3D frames, and synthetic Earth/Sol identity where applicable. The current Runtime View3D data is synthetic and these six scenarios do not produce an original Celestia business image.

The SelfTest uses two generated PNG files to prove the Runtime multi-image checkpoint framework. That fixture capability must not be described as real Runtime visual parity.

## Useful Options

```powershell
-SkipBuild
-SkipRuntimeSmoke
-CurrentBuildDir D:\path\to\build-mvc-sdl-rel
-ArtifactsRoot D:\path\to\.regression-artifacts\Celestia
-BaselineWorktree D:\path\to\celestia-compat-baseline-44ec265
-KeepTemp
```

Skipping a required check produces at least `warn=2`. `-SkipBuild` and `-SkipRuntimeSmoke` are diagnostic conveniences, not evidence for an automatic pass.

## Artifacts

Generated builds, screenshots, metrics, logs, reports, and contact sheets stay outside the repository by default:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\
  baselines\44ec265\
    baseline-manifest.json
    screenshots\baseline\
  runs\<timestamp>-<commit>-<mode>\
    machine-report.json
    machine-report.md
```

Fault injection fixtures are isolated under `%TEMP%\CelestiaCompatFaultInjection\FI-xx\` and never modify formal scenarios, YAML files, source code, or the formal baseline.

## Claim Boundary

A passing `Full` supports only the claim that no covered visible regression was found in the unified SDL path relative to the fixed baseline. It does not prove exhaustive Celestia feature parity, pixel-perfect equivalence, Qt/Win32 frontend parity, Model decoupling completion, or View3D migration completion.
