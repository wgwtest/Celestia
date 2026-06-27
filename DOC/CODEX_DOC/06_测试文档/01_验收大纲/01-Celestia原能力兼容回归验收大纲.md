# Celestia 原能力兼容回归验收大纲

日期：2026-06-27

## 1. 目标与边界

本验收机制用于回答一个明确问题：

```text
Celestia 在 MVC 解耦和多进程 runtime 演进后，普通统一 exe / in-process 主路径是否仍保留改造前 Celestia 的主要可见能力。
```

验收重点不是证明新的 M / C / V 多进程路径能力，而是防止解耦修改破坏原有未解耦系统的用户能力。多进程 runtime 仍作为补充回归项保留，但不能替代原 in-process 兼容性验证。

第一版覆盖 SDL 统一 exe：

```text
build-*/src/celestia/sdl/celestia-sdl.exe
```

当前本地构建未启用 Win32 / Qt 前端 exe，因此 Win32 / Qt 的脚本启动、URL 启动和菜单级交互验证列为后续扩展，不纳入第一版门禁。

## 2. 固定基线

基线提交固定为：

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

含义：

```text
14062ca refactor: decouple Celestia MVC boundaries 的父提交，
即本轮 MVC 代码级解耦开始前的本地 Celestia 能力基线。
```

后续每次回归应将当前提交与该基线构建产物进行对照，而不是只对当前分支做自测。

## 3. 推荐入口

计划新增统一入口：

```powershell
tools\regression\run_celestia_compat_regression.ps1
```

支持三种运行模式：

```powershell
# 第一次或基线失效时重新生成基线构建和基线截图
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline

# 日常提交前快速门禁
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick

# 阶段收口或大范围修改后的完整对照
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

## 4. 产物目录

大体积构建、截图和对比图不进入仓库，统一放在仓库外工作区：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\
  baselines\44ec265\
  runs\<timestamp>-<current-commit>\
```

仓库内只保留可复用脚本、场景脚本和文档：

```text
tools/regression/
  run_celestia_compat_regression.ps1
  scenarios/*.cel
  helpers/*.ps1
  helpers/*.py

DOC/CODEX_DOC/06_测试文档/
  01_验收大纲/
  02_验收入口/
  03_机测记录/
  05_验收结论/
```

每次 `Full` 运行应生成一份机器测试记录，放入：

```text
DOC/CODEX_DOC/06_测试文档/03_机测记录/
```

文件名采用时间戳前缀：

```text
YYYY-MM-DD-HHMMSS-Celestia原能力兼容回归-机测记录.md
```

## 5. 构建策略

脚本不应在主 checkout 上来回切换提交。基线使用独立 worktree：

```text
D:\WorkSpace\Codex\CeleNew\.worktrees\celestia-compat-baseline-44ec265
```

当前版本使用当前 checkout：

```text
D:\WorkSpace\Codex\CeleNew\Celestia
```

两边使用相同的 Visual Studio CMake / CTest 路径：

```powershell
$vsdev = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat'
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
```

第一版以 Release SDL 构建为主：

```text
ENABLE_SDL=ON
ENABLE_QT6=OFF
ENABLE_WIN=OFF
```

如果后续启用 Qt / Win32 前端，应新增前端专属矩阵，不混入 SDL 第一版门禁。

## 6. 场景注入机制

SDL 前端当前不是通过 `--url` 或 `--script` 选择启动场景。第一版不得为了测试修改程序入口。

推荐做法：

1. 为每个测试场景创建临时数据根。
2. 从构建产物的完整数据目录复制或镜像基础数据。
3. 在临时数据根写入临时 `celestia.cfg`。
4. 将 `InitScript` 指向对应的回归场景 `.cel`。
5. 设置 `CELESTIA_DATA_DIR` 指向临时数据根后启动 `celestia-sdl.exe`。

场景脚本负责稳定画面、截图和退出：

```text
wait { duration ... }
capture { filename "..." type "png" }
exit {}
```

截图由 Celestia 自身 `capture` 命令产生，避免外部窗口截图在焦点、DPI、遮挡、标题匹配上的不稳定。

## 7. 多场景截图矩阵

第一版至少包含以下 8 个 in-process SDL 视觉场景。

| 场景 ID | 覆盖能力 | 关键观察点 |
| --- | --- | --- |
| `earth-default` | 默认启动、Earth、星空、HUD、基础纹理 | 地球可见、HUD 文本可见、非黑屏 |
| `earth-clouds-orbits-labels` | 云图、轨道线、行星标签、render flags | 云图、轨道、标签同时出现 |
| `moon-close` | 月球纹理、近距离天体、选择 / goto / follow | 月球表面纹理、选择状态 |
| `saturn-rings` | 土星环、远距离太阳系对象、多卫星 | 环结构、行星本体、背景星空 |
| `asteroid-or-spacecraft` | 不规则模型或 extras-standard 模型资源 | Eros / ISS / Mir 等模型不缺失 |
| `starfield-constellations` | 星表、星名、星座线、星座名 | 恒星、星座线、标签显示 |
| `galaxy-deepsky` | 深空对象、银河 / 星系 / 星团渲染 | 深空对象可见且非纯黑 |
| `script-overlay-hud` | CEL 脚本、print 文本、overlay、字体 | 脚本文本和 overlay 正常 |

每个场景都要对基线版本和当前版本各产出一张截图。`Full` 模式至少生成 16 张原始截图，并生成汇总对照图。

后续可以扩展为多时间点截图，例如：

```text
earth-default/t+3s
earth-default/t+8s
saturn-rings/t+5s
saturn-rings/t+12s
```

但第一版优先保证场景数量和稳定性。

## 8. 非视觉回归矩阵

`Quick` 模式至少运行：

```powershell
cmd.exe /c "call `"$vsdev`" -arch=x64 -host_arch=x64 >NUL && `"$cmake`" --build build-mvc-baseline-rel --config Release --target unit && `"$ctest`" --test-dir build-mvc-baseline-rel -C Release --output-on-failure"

cmd.exe /c "call `"$vsdev`" -arch=x64 -host_arch=x64 >NUL && `"$cmake`" --build build-mvc-sdl-rel --config Release --target unit celestia-sdl && `"$ctest`" --test-dir build-mvc-sdl-rel -C Release --output-on-failure"

powershell -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1

powershell -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
```

并运行当前版本的 runtime smoke：

```powershell
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-2d-stdio.yaml
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-3d-stdio.yaml
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-2d-local-socket.yaml
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-3d-local-socket.yaml
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-switch-2d-to-3d-local-socket.yaml
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe --runtime-config DOC\CODEX_DOC\examples\runtime-switch-3d-to-2d-local-socket.yaml
```

## 9. 图像判定规则

不使用逐像素完全一致作为通过标准。OpenGL 驱动、字体、时间和硬件差异会导致稳定但无害的像素差异。

每张截图至少计算：

```text
width / height
nonBlackRatio
brightPixelRatio
colorfulPixelRatio
edgeDensity
averageColor
perceptualHash
```

机器判定分为三层：

1. 健康阈值：当前截图不能黑屏、不能极端单色、不能尺寸异常。
2. 场景阈值：每个场景有自己的最小亮度、彩色像素、边缘密度要求。
3. 相似阈值：当前截图与基线截图的 perceptual hash 距离或 SSIM 不能超过场景上限。

机器判定只负责拦截明显退化。视觉等价仍需要人工复核汇总图：

```text
baseline | current | diff
```

## 10. 报告内容

每次 `Full` 报告至少包含：

```text
baseline commit
current commit
build commands and exit codes
ctest summaries
MVC scan results
runtime smoke stdout/stderr
per-scene screenshot paths
per-scene image metrics
per-scene pass/warn/fail
residual process check
known exclusions
manual review checklist
```

如果出现 fail，报告必须明确区分：

```text
build failure
test failure
startup failure
script failure
screenshot missing
visual health failure
baseline/current visual drift
manual-review-required warning
```

## 11. 第一版不承诺自动覆盖

以下能力第一版不作为自动通过 / 失败门禁：

```text
任意菜单和对话框完整交互
任意键鼠导航组合
精确 FPS 性能等价
视频录制 / FFmpeg capture
第三方任意 add-on
全部 Celestia URL 启动路径
Qt / Win32 前端专项能力
天文数值精度全量校验
```

这些项目应作为后续专项验收扩展。

## 12. 推进顺序

建议按以下顺序落地：

1. 创建 `tools/regression/scenarios/` 并写 8 个稳定 `.cel` 场景。
2. 创建临时数据根生成逻辑，保证 `InitScript` 可切换。
3. 创建当前版本单侧截图 runner。
4. 创建图片指标统计 helper。
5. 创建 baseline worktree / baseline build 管理。
6. 创建 baseline-current 对照和汇总报告。
7. 接入 `Quick` / `Full` 两级门禁。
8. 将最新验收入口写入 `CODEX_START_HERE.md`。

## 13. 通过口径

`Quick` 通过条件：

```text
当前版本 build-mvc-baseline-rel / build-mvc-sdl-rel 构建通过
当前版本全量 CTest 通过
MVC dependency scan 通过
MVC CMake target scan 通过
当前版本 in-process SDL 多场景截图健康检查通过
当前版本 multi-process runtime smoke 通过
无残留 celestia host 进程
```

`Full` 通过条件：

```text
基线版本和当前版本均可构建
基线版本和当前版本均可完成同一套 in-process SDL 场景截图
所有场景当前截图满足健康阈值
所有场景 baseline-current 相似度在阈值内，或被人工记录为可接受差异
当前版本 CTest / scan / runtime smoke 均通过
报告产物完整
```

只有 `Full` 通过后，才能在阶段收口文档中表述：

```text
本阶段未发现普通统一 exe / in-process 主路径相对 MVC 改造前基线的可见能力退化。
```

不能表述为：

```text
所有 Celestia 功能均已自动验证。
所有视觉效果与原版逐像素一致。
Qt / Win32 前端也已完成同等验证。
```
