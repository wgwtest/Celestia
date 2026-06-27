# Celestia Compatibility Regression Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a repeatable regression harness that compares the pre-MVC Celestia baseline against the current checkout for unified SDL exe / in-process visual capability, while also retaining current MVC smoke gates.

**Architecture:** A PowerShell runner owns orchestration, build invocation, temporary data roots, process launching, and Markdown report generation. CEL scene scripts drive deterministic in-process screenshots through `InitScript`, and a Python helper computes image health/difference metrics from generated PNGs. A lightweight PowerShell self-test validates runner path handling and helper behavior without launching Celestia.

**Tech Stack:** PowerShell 5+, Python 3 standard library plus Pillow when available, existing Visual Studio CMake/CTest tools, Celestia CEL scripts, Git worktrees.

---

## File Structure

- Create `tools/regression/run_celestia_compat_regression.ps1`: main entry point for `InitBaseline`, `Quick`, `Full`, and `SelfTest`.
- Create `tools/regression/helpers/image_metrics.py`: PNG metrics and optional contact sheet generation.
- Create `tools/regression/helpers/Run-CelestiaCompatSelfTest.ps1`: fast self-test for script syntax, scenario inventory, metric helper behavior, and report path creation.
- Create `tools/regression/scenarios/*.cel`: eight deterministic screenshot scenarios.
- Create `tools/regression/README.md`: command usage and artifact locations for engineers.
- Modify `CODEX_START_HERE.md`: add the new validation entry point and clarify current branch/worktree context.
- Create `DOC/CODEX_DOC/06_测试文档/02_验收入口/01-Celestia原能力兼容回归入口.md`: user-facing acceptance entry.

## Task 1: Self-Test Shell

**Files:**
- Create: `tools/regression/helpers/Run-CelestiaCompatSelfTest.ps1`
- Create: `tools/regression/README.md`

- [ ] **Step 1: Write the failing self-test wrapper**

Create a helper that expects `tools/regression/run_celestia_compat_regression.ps1` and `tools/regression/helpers/image_metrics.py` to exist. Before those files exist, running the helper must fail with a clear missing-file message.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\helpers\Run-CelestiaCompatSelfTest.ps1
```

Expected red result before implementation:

```text
Missing regression runner
```

- [ ] **Step 2: Add README with the intended command surface**

Document these commands exactly:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

## Task 2: Scenario Pack

**Files:**
- Create: `tools/regression/scenarios/01-earth-default.cel`
- Create: `tools/regression/scenarios/02-earth-clouds-orbits-labels.cel`
- Create: `tools/regression/scenarios/03-moon-close.cel`
- Create: `tools/regression/scenarios/04-saturn-rings.cel`
- Create: `tools/regression/scenarios/05-asteroid-or-spacecraft.cel`
- Create: `tools/regression/scenarios/06-starfield-constellations.cel`
- Create: `tools/regression/scenarios/07-galaxy-deepsky.cel`
- Create: `tools/regression/scenarios/08-script-overlay-hud.cel`

- [ ] **Step 1: Create eight CEL scenarios**

Each script must set time rate to zero, select or center a representative target, wait long enough for rendering, call:

```text
capture { filename "$CAPTURE_FILE$" type "png" }
exit {}
```

The runner replaces `$CAPTURE_FILE$` with an absolute PNG path that uses forward slashes.

- [ ] **Step 2: Verify scenario inventory**

Run:

```powershell
Get-ChildItem tools\regression\scenarios -Filter *.cel | Measure-Object
```

Expected:

```text
Count : 8
```

## Task 3: Image Metrics Helper

**Files:**
- Create: `tools/regression/helpers/image_metrics.py`

- [ ] **Step 1: Add command-line contract first**

The helper supports:

```powershell
python tools\regression\helpers\image_metrics.py --self-test
python tools\regression\helpers\image_metrics.py --image path\to\scene.png --json path\to\metrics.json
python tools\regression\helpers\image_metrics.py --baseline path\a.png --current path\b.png --json path\to\compare.json
```

- [ ] **Step 2: Implement metrics with a Pillow fast path and stdlib fallback**

With Pillow installed, compute dimensions, average color, non-black ratio, bright pixel ratio, colorful pixel ratio, edge density, and dHash. Without Pillow, emit a JSON object with `available: false` and a clear reason instead of failing import.

- [ ] **Step 3: Run helper self-test**

Run:

```powershell
python tools\regression\helpers\image_metrics.py --self-test
```

Expected:

```text
image_metrics self-test passed
```

## Task 4: Main Regression Runner

**Files:**
- Create: `tools/regression/run_celestia_compat_regression.ps1`

- [ ] **Step 1: Add parameter model and path defaults**

The runner parameters are:

```powershell
param(
  [ValidateSet('SelfTest','InitBaseline','Quick','Full')] [string] $Mode = 'Quick',
  [string] $BaselineCommit = '44ec265659d2aa666cbf7546e36e4dde471d54ba',
  [string] $ArtifactsRoot,
  [string] $CurrentBuildDir,
  [string] $BaselineWorktree,
  [switch] $SkipBuild,
  [switch] $SkipRuntimeSmoke,
  [switch] $KeepTemp
)
```

- [ ] **Step 2: Implement build and scan commands**

Use existing Visual Studio tool paths from `CODEX_START_HERE.md`. `Quick` builds and tests the current checkout when `-SkipBuild` is not set; it always runs the two MVC scan scripts.

- [ ] **Step 3: Implement temporary data root generation**

Copy or symlink the repo content needed by `celestia.cfg`; write a temporary `celestia.cfg` with `InitScript` pointing to the generated scenario file and `ScriptScreenshotDirectory` set to the run screenshot directory.

- [ ] **Step 4: Implement current screenshot run**

For each `.cel` scenario, replace `$CAPTURE_FILE$`, set `CELESTIA_DATA_DIR`, launch `src\celestia\sdl\celestia-sdl.exe`, wait for exit, and verify the expected PNG exists.

- [ ] **Step 5: Implement baseline worktree handling**

`InitBaseline` creates or reuses `D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-compat-baseline-44ec265`, checks out the baseline commit, builds SDL, and captures baseline screenshots into `.regression-artifacts`.

- [ ] **Step 6: Implement Full comparison**

`Full` ensures baseline screenshots exist, captures current screenshots, runs `image_metrics.py` for each scene, and writes a Markdown machine report under `DOC/CODEX_DOC/06_测试文档/03_机测记录`.

## Task 5: Docs and Startup Guide

**Files:**
- Create: `DOC/CODEX_DOC/06_测试文档/02_验收入口/01-Celestia原能力兼容回归入口.md`
- Modify: `CODEX_START_HERE.md`

- [ ] **Step 1: Add acceptance entry doc**

Document when to use `Quick` versus `Full`, where artifacts are written, and the exact claim allowed after a passing `Full` run.

- [ ] **Step 2: Update startup guide**

Add the feature branch, runner path, and the newest validation command without changing the existing MVC architecture interpretation.

## Task 6: Verification and Commit

**Files:**
- All files above.

- [ ] **Step 1: Run PowerShell syntax parse**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command "$null = [System.Management.Automation.Language.Parser]::ParseFile('tools\regression\run_celestia_compat_regression.ps1',[ref]$null,[ref]$null)"
```

- [ ] **Step 2: Run self-test**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
```

- [ ] **Step 3: Run non-launch static checks**

Run:

```powershell
git diff --check
git status --short --branch
```

- [ ] **Step 4: Commit**

Run:

```powershell
git add CODEX_START_HERE.md DOC/CODEX_DOC/06_测试文档 tools/regression
git commit -m "test: add Celestia compatibility regression harness"
```
