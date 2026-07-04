# Celestia Step21 SolarSystem Model 边界交接

## 1. 当前状态

Step21 已按当前目标推进到可验证状态，但尚未提交。

本轮目标不是“Model 层完全解耦”，而是处理 Step20 后遗留的唯一 `src/celengine/model` 直接 include `src/celengine/adapter` 的 catalog 类族切片:

```text
src/celengine/model/universe.h -> celengine/adapter/solarsys.h
```

当前结果:

```text
SolarSystem / SolarSystemCatalog 已迁入 src/celengine/model/solarsystem.*
SolarSystemsBuilder 仍保留在 src/celengine/adapter/solarsys.*
Universe 改为 include celengine/model/solarsystem.h
model->adapter / model->view / model-view-symbol 三类扫描清零
Quick 回归通过，已产生 10 个截图场景
```

## 2. 主要文件

新增:

```text
src/celengine/model/solarsystem.h
src/celengine/model/solarsystem.cpp
test/scripts/test_mvc_model_adapter_boundary_clean.ps1
DOC/CODEX_DOC/04_研制计划/39-WBS-0.39-Celestia标准MVC解耦-Step21SolarSystem类族MVC拆解计划.md
DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-07-04-180127-Celestia-Step21-SolarSystem-model-boundary-machine-report.md
DOC/CODEX_DOC/07_过程文档/01_会话交接/2026-07-04-180127-Celestia-Step21-SolarSystem-model-boundary-handoff.md
```

修改:

```text
DOC/CODEX_DOC/02_设计说明/02-12-Celestia当前源码结构与Model类族分析第二版.md
src/celengine/CMakeLists.txt
src/celengine/adapter/solarsys.cpp
src/celengine/adapter/solarsys.h
src/celengine/model/universe.h
src/celruntime/assembly/runtimeassemblyrunner.cpp
tools/regression/run_celestia_compat_regression.ps1
```

## 3. 需要保留的判断边界

本轮可以说:

```text
SolarSystem catalog 类族已经按 StarDatabase / DSODatabase 的组织方向完成一轮最小拆分。
当前 model 目录内直接 adapter include 的硬依赖已经由新增扫描测试守住。
统一 exe 与多 runtime smoke 在 Quick 验证中通过。
```

本轮不能说:

```text
Model 层完全解耦完成。
StarDatabase / DSODatabase 已经彻底解耦。
adapter 已经不含 View3D 或渲染资源责任。
View3D 迁移已经完成。
```

## 4. 验证证据

TDD 红灯:

```text
test/scripts/test_mvc_model_adapter_boundary_clean.ps1
Exit code: 1
model->adapter findings must be cleared before Step21 can pass
Universe -> adapter/solarsys.h
```

TDD 绿灯:

```text
test/scripts/test_mvc_model_adapter_boundary_clean.ps1
Exit code: 0
MVC model adapter boundary is clean
```

构建:

```text
cmake --build build-mvc-sdl-rel --config Release --target unit celestia-sdl
Exit code: 0
```

Quick:

```text
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
Exit code: 0
```

Full baseline-current 截图对比:

```text
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full -SkipBuild
Exit code: 0
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-201008-33fae27-full\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-201008-33fae27-full\contact-sheet.png
```

Full 对比结论:

```text
10/10 scenes pass.
所有场景尺寸一致。
01/02/03/04/05/07/08 dHash Hamming = 0。
06-starfield-constellations dHash Hamming = 4, Average Color Distance = 0.003。
09-selection-follow-goto Average Color Distance = 0.001。
10-resource-fallback-missing Average Color Distance = 0.004。
已人工查看 contact sheet，未见明显可见退化。
```

Quick 报告:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-180127-33fae27-quick\machine-report.md
```

截图目录:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-180127-33fae27-quick\screenshots\current
```

CTest:

```text
100% tests passed, 0 tests failed out of 196
```

边界债务扫描:

```text
MVC boundary debt scan report: 63 finding(s)
adapter->runtime: 1
adapter->view: 12
runtime-model->app: 4
runtime-model->view: 2
runtime-model-projection: 44
```

## 5. 本轮额外修复的运行验证阻塞

首次 Quick 失败在:

```text
CTest #37 RuntimeAssemblyRunner starts the same 2D MVC session over stdio-pipe
CTest #40 RuntimeAssemblyRunner writes a trace with transport and exit codes
model receive timeout
```

根因:

```text
RuntimeAssemblyRunner 在 contentRoot 可解析到 run-full 时，会启动真实 ModelBackend。
真实模型冷加载可能超过默认 3000ms ready timeout。
Step13 的真实模型进程用例已经采用 8000ms ready timeout，说明真实数据根场景不能继续使用短超时。
```

处理:

```text
src/celruntime/assembly/runtimeassemblyrunner.cpp
当 dataRoot 非空时，将 host ready timeout 至少提升到 8000ms，将 shutdown timeout 至少提升到 5000ms。
```

验证:

```text
CTest #37 单独复跑: pass, 4.42 sec
CTest #40 单独复跑: pass, 4.24 sec
完整 Quick: pass
```

## 6. 下一步建议

建议下一步先不要进入 View3D 模板搬迁。更稳妥的顺序是:

1. 对 `adapter` 中仍绑定 View3D 资源的 Star / DSO / SolarSystem builder 与 render-assets 做横向拆解计划。
2. 对 `RealModelBackend` 依赖应用层加载入口、mesh/texture manager 的问题做单独审计。
3. 对 runtime 输出结构继续区分 `ModelSnapshot`、`SceneProjection`、`SceneFrame`，避免把当前 `ViewFrame` 误认为最终 Model 输出。

其中第 1 项可以作为 Step22 候选，但应先写审计和计划，不要直接做大规模搬迁。
