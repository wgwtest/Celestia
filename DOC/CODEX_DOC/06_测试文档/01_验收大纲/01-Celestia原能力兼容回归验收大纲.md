# Celestia 原能力兼容回归验收大纲

日期：2026-07-15

## 1. 目标与边界

本验收用于回答：

```text
Celestia 在 MVC 解耦和多进程 Runtime 演进后，普通 SDL 统一 exe / in-process 主路径在当前验证矩阵覆盖范围内，是否出现相对改造前固定基线的可见能力退化。
```

统一 exe 基线对照是原能力兼容性的主要证据。多进程 Runtime checkpoint 是补充证据，不能替代原始画面比较。

当前不覆盖 Qt/Win32 前端、全部菜单交互、像素级等价、任意第三方 add-on 或全部 Celestia 功能。

## 2. 固定基线

基线提交固定为：

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

基线使用独立 worktree 和独立构建目录，不在当前 checkout 上切换提交。有效基线必须同时具有：

1. 与固定提交一致的 `baseline-manifest.json`；
2. 与验证矩阵精确一致的场景/checkpoint/image 集合；
3. 与磁盘文件一致的 SHA-256；
4. 可读取且满足健康阈值的 PNG。

`Full` 只能读取和校验基线。创建或刷新基线必须显式运行 `InitBaseline`。

## 3. 统一入口

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\helpers\Run-CelestiaCompatFaultInjection.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Step18
```

调用方必须读取 `$LASTEXITCODE`：`pass=0`、`fail=1`、`warn=2`、`error=3`。required 检查被跳过时至少为 `warn=2`。

## 4. 场景注入

SDL 前端通过临时数据根注入场景，不为回归测试修改程序入口：

1. 从构建产物准备临时数据根；
2. 写入临时 `celestia.cfg`；
3. 将 `InitScript` 指向矩阵登记的 `.cel`；
4. 用 `CELESTIA_DATA_DIR` 启动 `celestia-sdl.exe`；
5. 由 Celestia 自身执行 `capture` 和 `exit`。

这样避免桌面截图受窗口焦点、DPI、遮挡和标题匹配影响。

## 5. 当前统一 exe 矩阵

场景和 checkpoint 数量由 `tools/regression/verification-matrix.json` 决定；当前统一 exe 组为十个场景、每场一个 `final` image checkpoint，因此 `Full` 当前产生十张 baseline、十张 current 和一张 contact sheet。

| 场景 ID | 主要覆盖 |
|---|---|
| `01-earth-default` | Earth、时间、选择、导航、表面、策略和可见性 |
| `02-earth-clouds-orbits-labels` | 云层、轨道、标签和开关组合后的最终画面 |
| `03-moon-close` | Moon 近景、选择、导航和表面 |
| `04-saturn-rings` | Saturn、本体、环、资源和背景 |
| `05-asteroid-or-spacecraft` | Eros、小天体表面、资源和标注 |
| `06-starfield-constellations` | 恒星、星表、星座线和标注 |
| `07-galaxy-deepsky` | Milky Way、深空目录和资源 |
| `08-script-overlay-hud` | CEL 脚本、反馈和 HUD 文本 |
| `09-selection-follow-goto` | 选择、goto/follow 命令触发、轨道和反馈 |
| `10-resource-fallback-missing` | 资源相关正常路径、表面、轨道和反馈 |

场景名称不能扩大其证据含义：`09` 不回读持续 follow 状态；`10` 没有制造资源缺失，因此不证明 missing/fallback。

## 6. Runtime 矩阵

六个 Runtime 场景按矩阵逐项执行 config、process、stdout/trace 和必要的 3D checkpoint：

```text
runtime-2d-stdio
runtime-2d-local-socket
runtime-3d-stdio
runtime-3d-local-socket
runtime-switch-2d-to-3d-local-socket
runtime-switch-3d-to-2d-local-socket
```

2D 场景不要求 3D payload。3D 和 switch 场景要求正数 frame count 和 synthetic Earth/Sol 身份。当前 Runtime 数据仍是 synthetic fixture，没有原业务图片，不能用于宣称原 View3D 画面已恢复。

## 7. 图像判定

不使用逐像素完全一致。每张图片至少检查：

```text
width / height
nonBlackRatio
brightPixelRatio
colorfulPixelRatio
edgeDensity
averageColor
dHash
```

判定层次：

1. 图片必须存在且可由 Pillow 读取；
2. 当前阈值要求至少 160x120，`nonBlackRatio >= 0.002`；
3. baseline/current 尺寸必须一致；
4. `dHashHamming > 30` 或 `averageColorDistance > 80` 产生 `warn=2`；
5. 人工复核 contact sheet，不能用人工判断覆盖机器 `fail/error`。

## 8. 非视觉检查

正式门禁还包括：

1. configure、build 和 CTest；
2. `scan_mvc_dependencies.ps1`；
3. `scan_cmake_targets.ps1`；
4. `test_mvc_model_adapter_boundary_clean.ps1`；
5. 六个 Runtime 场景；
6. Celestia/Host 残留进程检查。

每项形成独立 CheckResult。依赖阶段失败后，后续 required 项登记为 skipped，不能消失在报告中。

## 9. 报告

每次正式运行在独立 run 目录生成：

```text
machine-report.json
machine-report.md
```

JSON 是机器状态源。Markdown 从同一 RunSummary 生成。两者必须具有相同的 runId、mode、status、exitCode 和 check count。

报告必须能区分 build/test/scan/process/checkpoint/image/comparison/baseline/harness failure，并保留对应日志或 artifact 路径。

## 10. 自动化排除项

当前不作为自动通过条件：

```text
任意菜单和对话框完整交互
任意键鼠导航组合
精确 FPS 性能等价
视频录制和 FFmpeg capture
第三方任意 add-on
全部 Celestia URL
Qt/Win32 前端专项能力
天文数值精度全量校验
多进程原 View3D 业务画面
```

## 11. 通过口径

只有 `Full` 返回 0、基线 manifest 有效、所有 required 检查通过，并完成人工 contact sheet 复核后，才能表述：

```text
本次运行未发现普通 SDL 统一 exe / in-process 主路径在当前矩阵覆盖范围内相对固定基线的可见能力退化。
```

不能表述为：

```text
所有 Celestia 功能均已自动验证。
所有视觉效果与原版逐像素一致。
Qt/Win32 前端已完成同等验证。
Model 已完全解耦。
新 View3D 已完成迁移或视觉等价。
```

Step25 对验证工具本身的进一步要求见：

```text
DOC/CODEX_DOC/06_测试文档/01_验收大纲/03-Celestia-Step25验证机制加固验收大纲.md
```
