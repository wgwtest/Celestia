# Celestia Step20 Model 边界治理交接

## 1. 当前状态

Step20B、Step20C、Step20D 已按当前计划推进完成。当前尚未提交。

当前重要产物:

```text
DOC/CODEX_DOC/02_设计说明/02-11-Celestia-Runtime-Model输出分层说明.md
DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-07-03-125356-Celestia-Step20-Model-boundary-governance-machine-report.md
DOC/CODEX_DOC/07_过程文档/01_会话交接/2026-07-03-125356-Celestia-Step20-Model-boundary-governance-handoff.md
```

## 2. 已完成代码治理

```text
controller/simulation.h 去掉 View3D texture include。
adapter/*lifecycle.* 迁入 model/*lifecycle.*。
OrbitSampler 不再依赖 CurvePlot / CurvePlotSample。
BodyReferenceMark 成为 Model 侧参考标记基类。
View3D ReferenceMark 继承 BodyReferenceMark 并保留 render/isOpaque。
Marker/MarkerRepresentation 迁入 model/marker.*，并去掉 Renderer render 方法。
```

## 3. 当前扫描结果

```text
MVC boundary debt scan report: 64 finding(s)
model->view: 0
model-view-symbol: 0
```

剩余:

```text
adapter->runtime: 1
adapter->view: 12
model->adapter: 1
runtime-model->app: 4
runtime-model->view: 2
runtime-model-projection: 44
```

## 4. 已通过验证

```text
cmake --build build-mvc-sdl-rel --config Release --target celestia-sdl unit --parallel 8
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_boundary_debt.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_boundary_debt_scan.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

最近一次 Quick 通过记录:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-03-125931-a3cd97d-quick\machine-report.md
```

SelfTest 也已通过:

```text
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
Exit code: 0
```

## 5. 下一步建议

1. 用户确认后提交当前 Step20 成果。
2. 后续不要直接进入 View3D 模板搬迁；应先处理或明确保留 `model->adapter` 的 `Universe -> solarsys.h` 债务，以及 runtime 输出分层的下一阶段计划。
