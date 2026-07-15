# Celestia Step25 验证机制加固交接

## 1. 当前状态

Step25 九项任务已在当前工作树实施并完成正向、负向和人工图像验收，尚未提交、尚未推送。

```text
workspace: D:\WorkSpace\Codex\CeleNew\Celestia
branch: codex/celestia-mvc-step13-real-model-backend
HEAD: 0f733ef603b8db5ffc02b735c153d8302bfad3ed
worktree: dirty
```

本阶段没有修改 `src/`、`test/unit/` 或根 `CMakeLists.txt`。不要清理或覆盖既有 Step24 文档和本轮未提交成果。

## 2. Step25 交付物

主要实现：

```text
tools\regression\verification-matrix.json
tools\regression\helpers\CelestiaCompatRegression.psm1
tools\regression\helpers\Invoke-CelestiaCompatGateProbe.ps1
tools\regression\helpers\Run-CelestiaCompatFaultInjection.ps1
tools\regression\helpers\Run-CelestiaCompatSelfTest.ps1
tools\regression\run_celestia_compat_regression.ps1
tools\regression\README.md
```

正式文档：

```text
DOC\CODEX_DOC\04_研制计划\43-WBS-0.43-Celestia标准MVC解耦-Step25验证机制加固实施计划.md
DOC\CODEX_DOC\06_测试文档\01_验收大纲\03-Celestia-Step25验证机制加固验收大纲.md
DOC\CODEX_DOC\06_测试文档\03_机测记录\2026-07-15-201136-Celestia-Step25验证机制加固-机测记录.md
DOC\CODEX_DOC\07_过程文档\01_会话交接\2026-07-15-201136-Celestia-Step25验证机制加固-handoff.md
```

## 3. 最终验证证据

```text
SelfTest: pass/0
Fault injection: FI-00..FI-11 = 12/12 expected/actual matched
Quick: 2026-07-15-193233-0f733ef-quick = pass/0, 60 checks, 0 non-pass
Full: 2026-07-15-193548-0f733ef-full = pass/0, 96 checks, 0 non-pass
Step18: 2026-07-15-200722-0f733ef-step18 = pass/0, 60 checks, 0 non-pass
CTest in Full: 200/200 pass
Runtime: 6/6 process pass, 28/28 checkpoint pass
Unified images: 10/10 current pass
Baseline: 10/10 entries and SHA256 pass
Comparisons: 10/10 pass
Contact sheet: 20 entries, manually reviewed
Residual Celestia processes: none
```

Full 机器报告：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-15-193548-0f733ef-full\machine-report.json
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-15-193548-0f733ef-full\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-15-193548-0f733ef-full\contact-sheet.png
```

固定基线：

```text
commit: 44ec265659d2aa666cbf7546e36e4dde471d54ba
manifest: D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\baselines\44ec265\baseline-manifest.json
```

## 4. 能够证明和不能证明的边界

Step25 证明验证门槛能够可靠通过和可靠失败。

Step25 不证明 Model 已经达到 `M-L1`，不证明真实命令闭环，不证明真实跨进程 View3D 画面，也不授权直接开始 View3D 迁移。

Runtime 的 3D 检查使用 synthetic Earth fixture，只证明进程、传输、消息、身份和计数检查点。统一程序的 10 张图只证明各自最终截图检查点；场景名称不能替代实际覆盖范围。

`VG-01..VG-16` 仍全部开放。尤其是多 Observer 时间状态、完整导航、完整 Catalog/Star/DSO/Body/Surface/Atmosphere/Orbit/Annotation、per-View policy、Lua/CELX、真实资源损坏/fallback 和角色物理隔离尚未在 Step25 中实现。

## 5. 下一候选工作

下一候选是单独制定 `C-01` 时间/暂停权威状态实施计划。正确顺序是：

1. 从 `02-16` 的 `CAP-TIME`、`E-TIME-*`、`VG-01` 和 `C-01` 读取原业务链与验收要求。
2. 单独形成 C-01 实施计划，明确真实 Simulation/Observer 状态、命令端口、输出和双轨验证。
3. 先由用户评审计划，再决定是否修改代码。

未经用户确认，不开始 C-01 实现，不开始 View3D 迁移，不合并，不推送。

## 6. 复验命令

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\helpers\Run-CelestiaCompatFaultInjection.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Step18
```

`Full` 不得自动重建基线。只有明确执行 `InitBaseline` 才能产生或替换固定基线，并且必须核对 manifest 的 commit、精确场景集合和 SHA256。
