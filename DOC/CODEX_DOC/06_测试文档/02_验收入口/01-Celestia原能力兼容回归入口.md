# Celestia 原能力兼容回归入口

日期：2026-07-15

## 1. 用途

本入口验证两类内容：

1. 普通 SDL 统一 exe / in-process 主路径相对固定旧提交的已覆盖可见能力；
2. 当前六个多进程 Runtime 场景的进程、trace、clean shutdown 和矩阵 checkpoint。

它不是 Qt/Win32 前端验收，不是 Model 完全解耦验收，也不是新 View3D 与原始 3D 画面的等价验收。

## 2. 固定基线

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

该提交是 MVC 代码级改造开始前的本地能力基线。有效基线还必须具有通过校验的 `baseline-manifest.json`；仅有 PNG 文件不能视为有效基线。

## 3. 命令

脚本和隔离 fixture 自检：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
```

FI-00 至 FI-11 故障注入：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\helpers\Run-CelestiaCompatFaultInjection.ps1
```

显式构建并初始化固定基线：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
```

当前版本快速门禁：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

完整 baseline/current 对照：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

保留 Step18 声明边界的正式入口：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Step18
```

命令结束后必须读取：

```powershell
$LASTEXITCODE
```

| 退出码 | 状态 | 处理 |
|---:|---|---|
| 0 | `pass` | 已覆盖检查通过 |
| 1 | `fail` | 停止后续迁移并定位失败检查 |
| 2 | `warn` | 人工复核，不得自动当作通过 |
| 3 | `error` | 验证结果不可用，先修复工具或环境 |

## 4. 当前覆盖

统一 exe 场景由 `tools/regression/verification-matrix.json` 登记，当前为：

```text
01-earth-default
02-earth-clouds-orbits-labels
03-moon-close
04-saturn-rings
05-asteroid-or-spacecraft
06-starfield-constellations
07-galaxy-deepsky
08-script-overlay-hud
09-selection-follow-goto
10-resource-fallback-missing
```

当前每场一个 `final` 图片 checkpoint。`09` 不证明持续 follow；`10` 没有制造资源缺失，不证明 missing/fallback。

Runtime 当前为：

```text
runtime-2d-stdio
runtime-2d-local-socket
runtime-3d-stdio
runtime-3d-local-socket
runtime-switch-2d-to-3d-local-socket
runtime-switch-3d-to-2d-local-socket
```

3D 和 switch 场景中的 Earth/Sol 身份是 synthetic model fixture。当前真实 Runtime 场景没有原 Celestia 业务图片。

## 5. 产物

默认根目录：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\
```

正式 run 目录包含：

```text
runs\<timestamp>-<commit>-<mode>\machine-report.json
runs\<timestamp>-<commit>-<mode>\machine-report.md
```

JSON 是机器状态源；Markdown 由同一 RunSummary 生成。报告存在不代表通过。

## 6. 允许结论

只有 `Full=0` 且 contact sheet 完成人工复核后，才允许写：

```text
本次运行未发现普通 SDL 统一 exe / in-process 主路径在当前十场景矩阵内相对固定基线的可见能力退化。
```

不能写：

```text
所有 Celestia 功能均已验证。
多进程 View3D 已恢复原始 3D 画面。
Model 层已经完全解耦。
Qt / Win32 前端已完成同等验证。
```

完整验收规则见：

```text
DOC/CODEX_DOC/06_测试文档/01_验收大纲/03-Celestia-Step25验证机制加固验收大纲.md
```
