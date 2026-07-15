# Celestia Step25 验证机制加固验收大纲

日期：2026-07-15

## 1. 验收目标

Step25 验收对象是兼容回归工具本身，目标是确认：

1. 场景身份、检查点和能力边界由同一份验证矩阵登记；
2. 每个检查产生结构化 CheckResult；
3. JSON、Markdown、控制台结论和进程退出码来自同一份 RunSummary；
4. 固定旧提交基线具有可验证的场景/checkpoint 身份和 SHA-256；
5. 已知故障能够稳定阻断自动流程；
6. 当前工具可以承载 Runtime 多场景、多检查点和多图片验证。

Step25 不是 Model 解耦验收，也不是 View3D 迁移验收。

## 2. 状态与退出码

| 状态 | 退出码 | 含义 |
|---|---:|---|
| `pass` | 0 | 已执行的 required 检查全部通过 |
| `fail` | 1 | 被测程序、artifact 或已知验证项失败 |
| `warn` | 2 | 证据不足、图片漂移或 required 检查被显式跳过，需要人工复核 |
| `error` | 3 | 验证工具、矩阵、依赖或运行环境自身不可用 |

调用方必须读取 `$LASTEXITCODE`。报告文件存在不表示通过，`warn=2` 也不得被自动流程当作成功。

## 3. 验证矩阵

唯一登记文件：

```text
tools/regression/verification-matrix.json
```

矩阵必须拒绝重复场景 ID、重复 checkpoint ID、未知 kind、重复 artifact、零 checkpoint、路径逃逸，以及实际 `.cel` 文件集合与登记集合不一致。

### 3.1 统一 exe 场景

当前登记十个场景，每场一个 `final` 图片 checkpoint：

| ID | 当前有效能力声明 | 当前不能据此声明的内容 |
|---|---|---|
| `01-earth-default` | 时间、选择、导航、天体、表面、策略和可见性 | 不回读状态；不证明持续 synchronous follow |
| `02-earth-clouds-orbits-labels` | Earth 基础能力、云层、轨道和标注 | 最终合成图不能分别证明每个开关状态 |
| `03-moon-close` | Moon 选择、导航、表面和可见性 | 不证明复杂 journey 或持续 tracking |
| `04-saturn-rings` | Saturn、环资源、表面和大气相关链路 | 不证明资源缺失和多时刻运动 |
| `05-asteroid-or-spacecraft` | Eros、小天体资源和标注 | 不证明 spacecraft、CPU pick、Location projector 或 GPU cache 边界 |
| `06-starfield-constellations` | 星表、恒星、星座和标注 | 不分别证明近星/远星内部链路 |
| `07-galaxy-deepsky` | Milky Way、深空目录和资源 | 不证明四类 DSO 各自资源和画法 |
| `08-script-overlay-hud` | CEL 脚本、反馈和 HUD 文本 | 不证明 image/video Overlay 或真实 Session 写入归属 |
| `09-selection-follow-goto` | 选择、导航、脚本、轨道和标注 | 不证明持续 follow 或完整 goto journey |
| `10-resource-fallback-missing` | 天体、表面、资源、轨道和反馈 | 没有制造资源缺失，不证明 missing/fallback |

`Full` 当前产生十张 baseline、十张 current 和一张 contact sheet。数量由矩阵 checkpoint 决定，后续增加 checkpoint 时不得继续硬编码十张。

### 3.2 Runtime 场景

当前登记六个场景：

| ID | required 检查 |
|---|---|
| `runtime-2d-stdio` | config、process exit 0、trace、clean shutdown |
| `runtime-2d-local-socket` | config、process exit 0、trace、clean shutdown |
| `runtime-3d-stdio` | 基础项、正数 frame、synthetic Earth/Sol、body/star/resource 数量 |
| `runtime-3d-local-socket` | 与 3D stdio 相同，保留独立证据 |
| `runtime-switch-2d-to-3d-local-socket` | clean shutdown、trace、至少一个 3D frame、synthetic Earth/Sol |
| `runtime-switch-3d-to-2d-local-socket` | clean shutdown、trace、至少一个 3D frame、synthetic Earth/Sol |

这里的 Earth/Sol 数据来自 synthetic model fixture。六个真实 Runtime 场景当前没有原 Celestia 业务画面，不得据此声明 RealModelBackend、原 View3D 或真实 Catalog 已完成视觉等价。

## 4. 固定基线

基线提交：

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

有效基线必须同时满足：

1. `baseline-manifest.json` 可解析且 schemaVersion 为 1；
2. manifest 的提交号等于固定提交；
3. 矩阵中的 `(scenarioId, checkpointId, artifact)` 与 manifest 精确相等；
4. manifest 与磁盘 PNG 集合精确相等，不缺图、不多图；
5. 不存在重复键、未知场景或重复路径；
6. 每个 SHA-256 与当前文件一致，格式为 64 位小写十六进制；
7. 每张图片可由 `image_metrics.py` 读取，尺寸和非黑比例达标。

`Full` 只读取并校验基线。基线无效时必须返回 1，并提示显式执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
```

`Full` 不得自动创建、覆盖或补齐基线。

## 5. 多图片框架

隔离 SelfTest 使用 Pillow 生成两张 160x120 非黑 PNG，并验证：

1. 两图都存在时 pass；
2. 删除第一张时准确定位第一 checkpoint；
3. 删除第二张时准确定位第二 checkpoint；
4. 全黑图不能通过 nonBlackRatio；
5. 两个 checkpoint 不能登记相同 artifact。

该结果只证明框架可以执行 Runtime 多图片检查。它不表示六个真实 Runtime 场景已经产生业务图片。

## 6. 故障注入

| ID | 注入 | 预期 |
|---|---|---|
| `FI-00` | 全部正向 fixture | `pass/0` |
| `FI-01` | 临时矩阵缺少场景 ID | `fail/1` |
| `FI-02` | 临时场景目录增加未登记 `.cel` | `fail/1` |
| `FI-03` | 临时基线缺一张 PNG | `fail/1` |
| `FI-04` | 临时基线 hash 不一致 | `fail/1` |
| `FI-05` | 当前图片为全黑 PNG | `fail/1` |
| `FI-06` | Runtime stdout 缺 clean shutdown | `fail/1` |
| `FI-07` | Runtime frame count 为 0 | `fail/1` |
| `FI-08` | 被测子进程退出 17 | `fail/1`，保留原始退出码 |
| `FI-09` | 图片差异达到 warn 阈值 | `warn/2` |
| `FI-10` | gate probe 读取损坏 JSON | `error/3` |
| `FI-11` | Runtime 双图缺任一张 | `fail/1` |

除 FI-00 外，每项还必须执行 clean variant 并回到 `pass/0`。故障套件自身必须进行反向检查：故意写错一个 ExpectedExitCode 后，runner 必须非零退出并报告 expected/actual mismatch。

## 7. 报告一致性

每个正式模式必须在同一 run 目录生成：

```text
machine-report.json
machine-report.md
```

JSON 是机器状态源。Markdown 顶部的 runId、mode、status、exitCode 和 check count 必须逐项来自同一 RunSummary。最外层未捕获异常必须形成 `harness/unhandled-exception`，写报告并返回 3。

## 8. Contact Sheet 人工检查

`Full` 完成后，人工检查 contact sheet：

| 检查项 | 结果记录 |
|---|---|
| 十个场景均有 baseline/current 成对画面 | 待当次运行填写 |
| 无黑屏、空白帧、错误窗口或截断画面 | 待当次运行填写 |
| Earth、Moon、Saturn、Eros、星空和 Milky Way 主体可辨识 | 待当次运行填写 |
| 云层、轨道、标签、环、HUD 等当前声明要素无明显缺失 | 待当次运行填写 |
| 图片差异与机器 warn/fail 一致，没有被报告漏记 | 待当次运行填写 |

人工通过不能覆盖机器 `fail` 或 `error`。机器 `warn` 只能在记录原因和证据后人工处置。

## 9. 完成边界

Step25 完成只允许说明：

```text
Celestia 兼容回归工具已具备矩阵驱动、统一状态和退出码、精确基线、Runtime checkpoint、多图片 fixture 与故障注入能力。
```

Step25 完成不能说明：

1. Model 层已经完全解耦；
2. Controller 责任归属已经全部完成；
3. 原 View3D 已迁移到模板 View；
4. 多进程 View3D 已达到原始 3D 画面；
5. 所有 Celestia 功能、Qt/Win32 前端或像素级视觉等价均已验证。
