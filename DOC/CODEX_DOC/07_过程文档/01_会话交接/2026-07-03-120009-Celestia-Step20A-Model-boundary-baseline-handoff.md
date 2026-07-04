# Celestia Step20A Model 边界基线交接

时间: 2026-07-03 12:00:09

分支: `codex/celestia-mvc-step13-real-model-backend`

## 1. 本轮完成

本轮按 Step20 计划推进 Step20A，未修改 C++ 源码。

新增产物:

```text
DOC/CODEX_DOC/02_设计说明/02-10-Celestia-Model边界实体归属矩阵.md
DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-07-03-120009-Celestia-Step20A-Model-boundary-baseline-machine-report.md
DOC/CODEX_DOC/07_过程文档/01_会话交接/2026-07-03-120009-Celestia-Step20A-Model-boundary-baseline-handoff.md
```

## 2. 当前结论

可以说明:

```text
Model 边界实体归属矩阵第一版已建立。
Step20A 机器扫描基线已固定。
当前边界债务报告为 88 项。
常规 MVC dependency scan 和 CMake target scan 通过。
边界债务扫描脚本自测通过。
```

不能说明:

```text
Model 层已经完全解耦。
Model 层已经完全不需要再动。
View3D 模板迁移已经具备直接执行条件。
本轮重新验证了统一 exe 原能力。
```

## 3. 验证结果

执行过:

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
MVC boundary debt scan report: 88 finding(s)
MVC boundary debt scan self-test passed
```

未执行 C++ 构建、CTest 和 Quick 回归，因为本轮没有修改 C++ 源码。

## 4. 下一步建议

建议下一步进入 Step20B，先处理最低风险的 Controller 反向依赖:

```text
src/celengine/controller/simulation.h
  #include <celengine/view3d/texture.h>
```

Step20B 必须执行:

```powershell
git diff --check
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_boundary_debt.ps1
cmd.exe /d /s /c "call ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 >nul && ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --build ""D:\WorkSpace\Codex\CeleNew\Celestia\build-mvc-sdl-rel"" --config Release --target celestia-sdl unit --parallel 8"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

Step20B 不应同时处理生命周期事件、`OrbitSampler` 或 `ReferenceMark`；那些属于中高风险后续切片。

