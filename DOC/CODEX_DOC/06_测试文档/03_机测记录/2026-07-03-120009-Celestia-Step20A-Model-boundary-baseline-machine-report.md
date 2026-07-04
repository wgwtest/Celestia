# Celestia Step20A Model 边界基线机测记录

记录时间: 2026-07-03 12:00:09

分支: `codex/celestia-mvc-step13-real-model-backend`

本轮目标: 为 Step20A 建立 Model 边界实体归属矩阵和机器扫描基线。本轮不修改 C++ 源码，不移动文件，不调整 CMake target。

## 1. 验证范围

本轮验证覆盖:

```text
tools/mvc/scan_mvc_dependencies.ps1
tools/mvc/scan_cmake_targets.ps1
tools/mvc/scan_mvc_boundary_debt.ps1
test/scripts/test_mvc_boundary_debt_scan.ps1
```

本轮未执行:

```text
C++ 编译
CTest
Quick 截图回归
```

原因: 本轮是 Step20A 文档和扫描基线阶段，未修改 C++ 程序代码。后续 Step20B 开始做低风险源码清理时，必须执行编译和 Quick 回归。

## 2. 命令和结果

### 2.1 MVC dependency scan

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
```

结果:

```text
MVC dependency scan passed
```

结论:

```text
现有 MVC dependency scan 通过。
该扫描不覆盖全部 Model 边界债务，不能单独证明 Model 层已解耦完成。
```

### 2.2 MVC CMake target scan

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
```

结果:

```text
MVC CMake target scan passed
```

结论:

```text
现有 CMake target 边界扫描通过。
这说明目标组织扫描未发现当前规则下的问题，但不等同于源码职责完全归位。
```

### 2.3 MVC boundary debt scan

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_boundary_debt.ps1
```

结果:

```text
MVC boundary debt scan report: 88 finding(s)

adapter->runtime: 1
adapter->view: 12
controller->view: 1
model->adapter: 4
model->view: 2
model-view-symbol: 18
runtime-model->app: 4
runtime-model->view: 2
runtime-model-projection: 44
```

代表性发现:

```text
src\celengine\adapter\sceneviewmodel.h -> celruntime/viewframe.h
src\celengine\controller\simulation.h -> celengine/view3d/texture.h
src\celengine\model\body.cpp -> celengine/adapter/bodylifecycle.h
src\celengine\model\body.cpp -> celengine/view3d/referencemark.h
src\celengine\model\orbitsampler.h -> celengine/view3d/curveplot.h
src\celengine\model\universe.h -> celengine/adapter/solarsys.h
src\celruntime\model\realmodelbackend.cpp -> celengine/view3d/meshmanager.h
src\celruntime\model\realmodelbackend.cpp -> celestia/load*.h
src\celruntime\model\sceneextractor.* -> ViewFrame / SceneFrame / render state 投影结构
```

结论:

```text
88 项是 Step20A 当前基线，不是本轮新增退化。
报告模式返回 0，当前用于审计和治理排序。
后续不能把扫描清零作为唯一验收标准；必须结合实体归属矩阵、代码治理和回归验证。
```

### 2.4 boundary debt scan self-test

命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_boundary_debt_scan.ps1
```

结果:

```text
MVC boundary debt scan self-test passed
```

结论:

```text
边界债务扫描脚本的报告模式和自测入口可用。
```

## 3. 本轮产物

新增:

```text
DOC/CODEX_DOC/02_设计说明/02-10-Celestia-Model边界实体归属矩阵.md
DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-07-03-120009-Celestia-Step20A-Model-boundary-baseline-machine-report.md
```

已有计划依据:

```text
DOC/CODEX_DOC/04_研制计划/38-WBS-0.38-Celestia标准MVC解耦-Step20Model层进一步解耦计划.md
```

## 4. 阶段判断

本轮可以说明:

```text
Step20A 已建立 Model 边界实体归属矩阵第一版。
Step20A 已固定当前边界债务扫描基线。
Step20B 可以从低风险 include 清理开始。
```

本轮不能说明:

```text
Model 层已经完全解耦。
Model 层已经完全不需要再动。
View3D 模板迁移已经具备直接执行条件。
统一 exe 原能力已经在本轮重新验证。
```

## 5. 后续建议

建议下一轮进入 Step20B:

1. 删除 `src/celengine/controller/simulation.h` 中疑似未使用的 `#include <celengine/view3d/texture.h>`。
2. 运行边界债务扫描，确认 `controller->view` 分类变化。
3. 执行 C++ 构建和相关 CTest。
4. 执行 Quick 回归，确认统一 exe 原能力不退化。

