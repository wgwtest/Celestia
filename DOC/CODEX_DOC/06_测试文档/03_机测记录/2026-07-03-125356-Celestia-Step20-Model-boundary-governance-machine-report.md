# Celestia Step20 Model 边界治理机测记录

记录时间: 2026-07-03 12:53:56

## 1. 本轮范围

本轮从 Step20A 基线继续执行 Step20B、Step20C、Step20D:

1. Step20B: 低风险 include 清理。
2. Step20C-1: 生命周期事件从 Adapter 迁入 Model。
3. Step20C-2: `OrbitSampler` 与 View3D `CurvePlot` 拆分。
4. Step20D-1: `ReferenceMark` / `Marker` 表现边界治理。
5. Step20D-2: Runtime Model 输出分层说明。

## 2. 关键源码改动

```text
src/celengine/controller/simulation.h
src/celengine/model/bodylifecycle.*
src/celengine/model/stardetailslifecycle.*
src/celengine/model/nebulalifecycle.*
src/celengine/model/orbitsampler.h
src/celengine/model/bodyreferencemark.h
src/celengine/model/marker.*
src/celengine/model/body.*
src/celengine/model/universe.h
src/celengine/view3d/referencemark.h
src/celengine/view3d/render.cpp
src/celengine/view3d/glmarker.cpp
src/celengine/CMakeLists.txt
tools/mvc/scan_mvc_boundary_debt.ps1
test/scripts/test_mvc_boundary_debt_scan.ps1
test/unit/mvc_step3_contract_test.cpp
```

## 3. 边界扫描结果

Step20A 基线:

```text
MVC boundary debt scan report: 88 finding(s)
```

Step20B 后:

```text
MVC boundary debt scan report: 87 finding(s)
controller->view: 0
```

Step20C-1 后:

```text
MVC boundary debt scan report: 84 finding(s)
model->adapter: 1
```

Step20C-2 后:

```text
MVC boundary debt scan report: 78 finding(s)
model->view: 1
```

Step20D-1 / Step20D-2 后:

```text
MVC boundary debt scan report: 64 finding(s)
model->view: 0
model-view-symbol: 0
```

当前剩余分类:

```text
adapter->runtime: 1
adapter->view: 12
model->adapter: 1
runtime-model->app: 4
runtime-model->view: 2
runtime-model-projection: 44
```

## 4. 已执行验证

### 4.1 构建

命令:

```powershell
cmd.exe /d /s /c "call ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 >nul && ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --build ""D:\WorkSpace\Codex\CeleNew\Celestia\build-mvc-sdl-rel"" --config Release --target celestia-sdl unit --parallel 8"
```

结果:

```text
Exit code: 0
```

### 4.2 MVC 扫描

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_boundary_debt.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_boundary_debt_scan.ps1
```

结果:

```text
MVC dependency scan passed
MVC CMake target scan passed
MVC boundary debt scan self-test passed
MVC boundary debt scan report: 64 finding(s)
```

### 4.3 Quick 回归

已通过的 Quick 记录:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-123117-a3cd97d-quick\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-123619-a3cd97d-quick\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-124024-a3cd97d-quick\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-125028-a3cd97d-quick\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-125515-a3cd97d-quick\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-125931-a3cd97d-quick\machine-report.md
```

说明:

```text
2026-07-03-123117 前一次 Quick 曾在 CTest #37 出现 stdio-pipe model receive timeout。
单独重跑 #37 连续 3 次通过，随后完整 Quick 通过。
2026-07-03-125931 为 Step20 收尾后重新执行的最终 Quick 通过记录。
```

### 4.4 SelfTest

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
```

结果:

```text
Exit code: 0
```

## 5. 阶段结论

本轮可以说明:

```text
src/celengine/model 当前已清除直接 View3D/renderer include。
生命周期事件、OrbitSampler、ReferenceMark、Marker 已完成一轮实质边界治理。
统一 exe 和 runtime smoke 在 Quick 回归中通过。
```

本轮不能说明:

```text
Model 层已经完全解耦。
Runtime Model 输出已经完全分层。
adapter 目录已经完成职责拆分。
View3D 模板迁移已经完成。
```

## 6. 剩余风险

1. `Universe` 仍包含 `adapter/solarsys.h`，这是真实剩余 `model->adapter` 债务。
2. `adapter` 内部仍混有 View3D 资源绑定、加载、投影和 picking 能力。
3. `RealModelBackend` 仍直接依赖应用层加载入口和 View3D mesh/texture manager。
4. `ViewFrame` 当前仍是过渡结构，不是纯 `ModelSnapshot`。
