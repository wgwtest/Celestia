# Celestia Step21 SolarSystem Model 边界机测记录

记录时间: 2026-07-04 18:01:27

## 1. 本轮范围

本轮 Step21 只处理一个明确切片:

```text
消除 src/celengine/model/universe.h 直接 include src/celengine/adapter/solarsys.h 的 model->adapter 硬依赖。
```

本轮不证明 Model 层已经完全解耦，也不处理 Star / DSO builder、render-assets、runtime 输出分层、View3D 模板迁移等剩余问题。

## 2. 关键源码改动

```text
src/celengine/model/solarsystem.h
src/celengine/model/solarsystem.cpp
src/celengine/model/universe.h
src/celengine/adapter/solarsys.h
src/celengine/adapter/solarsys.cpp
src/celengine/CMakeLists.txt
src/celruntime/assembly/runtimeassemblyrunner.cpp
tools/regression/run_celestia_compat_regression.ps1
test/scripts/test_mvc_model_adapter_boundary_clean.ps1
DOC/CODEX_DOC/02_设计说明/02-12-Celestia当前源码结构与Model类族分析第二版.md
DOC/CODEX_DOC/04_研制计划/39-WBS-0.39-Celestia标准MVC解耦-Step21SolarSystem类族MVC拆解计划.md
```

核心变化:

1. 新增 `model/solarsystem.*`，承载 `SolarSystem` 和 `SolarSystemCatalog`。
2. `adapter/solarsys.*` 保留 `SolarSystemsBuilder`、`.ssc` 解析、资源路径和 render assets 相关逻辑。
3. `model/universe.h` 改为 include `celengine/model/solarsystem.h`。
4. `run_celestia_compat_regression.ps1` 纳入 `test_mvc_model_adapter_boundary_clean.ps1`。
5. `RuntimeAssemblyRunner` 在启用真实 `run-full` 数据根时，把 host ready / shutdown 超时窗口提升到 Step13 已使用的真实模型加载窗口，避免真实模型冷加载导致 `stdio-pipe` model ready 超时。

## 3. TDD 红绿记录

### 3.1 红灯

新增测试:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_model_adapter_boundary_clean.ps1
```

首次运行结果:

```text
Exit code: 1
model->adapter findings must be cleared before Step21 can pass
src\celengine\model\universe.h:23: model source includes adapter dependency: #include <celengine/adapter/solarsys.h>
```

### 3.2 绿灯

完成 `SolarSystem` 类族拆分后，复跑:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_model_adapter_boundary_clean.ps1
```

结果:

```text
Exit code: 0
MVC model adapter boundary is clean
```

## 4. 构建与单元测试

构建命令:

```powershell
cmd.exe /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >NUL && "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "D:\WorkSpace\Codex\CeleNew\Celestia\build-mvc-sdl-rel" --config Release --target unit celestia-sdl'
```

结果:

```text
Exit code: 0
```

Quick 回归内置 CTest 结果:

```text
100% tests passed, 0 tests failed out of 196
Total Test time (real) = 55.92 sec
```

## 5. 运行与截图验证

Quick 命令:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

结果:

```text
Exit code: 0
machine-report: D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-180127-33fae27-quick\machine-report.md
```

Quick 覆盖:

```text
build: pass
ctest: 196/196 pass
scan_mvc_dependencies: pass
scan_cmake_targets: pass
test_mvc_model_adapter_boundary_clean: pass
runtime smoke: 6/6 pass
screenshots: 10/10 pass
```

补充 Full 对比:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full -SkipBuild
```

结果:

```text
Exit code: 0
machine-report: D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-201008-33fae27-full\machine-report.md
contact-sheet: D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-201008-33fae27-full\contact-sheet.png
status: pass
```

Full 对比结果:

| Scene | Status | dHash Hamming | Average Color Distance |
| --- | --- | ---: | ---: |
| `01-earth-default` | `pass` | 0 | 0.0 |
| `02-earth-clouds-orbits-labels` | `pass` | 0 | 0.0 |
| `03-moon-close` | `pass` | 0 | 0.0 |
| `04-saturn-rings` | `pass` | 0 | 0.0 |
| `05-asteroid-or-spacecraft` | `pass` | 0 | 0.0 |
| `06-starfield-constellations` | `pass` | 4 | 0.003 |
| `07-galaxy-deepsky` | `pass` | 0 | 0.0 |
| `08-script-overlay-hud` | `pass` | 0 | 0.0 |
| `09-selection-follow-goto` | `pass` | 0 | 0.001 |
| `10-resource-fallback-missing` | `pass` | 0 | 0.004 |

人工抽查:

```text
已打开 contact-sheet.png 查看 baseline/current 逐对截图。
月球、土星环、星座线、HUD、轨道线等关键场景未见明显可见退化。
```

截图目录:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-04-180127-33fae27-quick\screenshots\current
```

截图场景:

| Scene | Status |
| --- | --- |
| `01-earth-default` | `pass` |
| `02-earth-clouds-orbits-labels` | `pass` |
| `03-moon-close` | `pass` |
| `04-saturn-rings` | `pass` |
| `05-asteroid-or-spacecraft` | `pass` |
| `06-starfield-constellations` | `pass` |
| `07-galaxy-deepsky` | `pass` |
| `08-script-overlay-hud` | `pass` |
| `09-selection-follow-goto` | `pass` |
| `10-resource-fallback-missing` | `pass` |

Runtime smoke:

| Config | Status |
| --- | --- |
| `runtime-2d-stdio.yaml` | `pass` |
| `runtime-3d-stdio.yaml` | `pass` |
| `runtime-2d-local-socket.yaml` | `pass` |
| `runtime-3d-local-socket.yaml` | `pass` |
| `runtime-switch-2d-to-3d-local-socket.yaml` | `pass` |
| `runtime-switch-3d-to-2d-local-socket.yaml` | `pass` |

## 6. 扫描结果

Step21 验收扫描:

```text
MVC model adapter boundary is clean
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

本轮新增测试只要求以下三类清零:

```text
model->adapter: 0
model->view: 0
model-view-symbol: 0
```

其余 MVC 边界债务仍需要后续分阶段处理，不属于本轮清零范围。

## 7. 本轮发现并修复的验证阻塞

首次 Quick 在 CTest #37 / #40 失败:

```text
RuntimeAssemblyRunner starts the same 2D MVC session over stdio-pipe
RuntimeAssemblyRunner writes a trace with transport and exit codes
model receive timeout
```

定位结论:

```text
Step10 用例通过 RuntimeAssemblyRunner 自动解析到 build/run-full 后，会启动真实 ModelBackend。
真实模型冷加载耗时超过默认 3000ms ready timeout。
Step13 真实模型进程用例已显式使用 8000ms ready timeout。
```

处理方式:

```text
RuntimeAssemblyRunner 在解析到真实 dataRoot 时，将 hostReadyTimeoutMilliseconds 至少提升到 8000，将 shutdownTimeoutMilliseconds 至少提升到 5000。
```

复跑结果:

```text
CTest #37: Passed, 4.42 sec
CTest #40: Passed, 4.24 sec
Quick: pass
```

## 8. 阶段结论

本轮可以说明:

```text
SolarSystem / SolarSystemCatalog 已从 adapter 头文件中拆入 model。
Universe 不再直接 include adapter/solarsys.h。
src/celengine/model 的 model->adapter、model->view、model-view-symbol 三类硬边界扫描已清零。
统一 SDL exe / runtime smoke / 10 场景截图 Quick 回归通过。
```

本轮不能说明:

```text
Model 层已经完全解耦。
Star / DSO 的 builder/render-assets 债务已经清除。
adapter 目录已经完成职责拆分。
Runtime Model 输出已经完成最终分层。
View3D 模板迁移已经可以直接开始或已经完成。
```
