# Celestia 统一 exe 原能力保真阶段结论

日期：2026-06-27

## 1. 当前结论

基于 `2026-06-27-155813` 的 Full 回归机测，当前可以写入阶段性结论：

```text
在当前 8 个自动化视觉场景和 6 个 runtime smoke 覆盖范围内，未发现普通 SDL 统一 exe / in-process 主路径相对 MVC 改造前固定基线的可见能力退化。
```

这个结论只覆盖普通 SDL 统一 exe 的 in-process 主路径，不覆盖 Qt / Win32 前端，也不等同于跨进程 OpenGL3D View 已达到历史 Renderer 完整视觉等价。

## 2. “统一 exe”在本结论中的含义

本结论中的“统一 exe”指：

```text
build-mvc-sdl-rel\src\celestia\sdl\celestia-sdl.exe
```

在不传入 `--runtime-config`、不传入 `--mvc-mode=multi-process` 时，它走普通 in-process 应用路径：

```text
celestia-sdl.exe
  -> CelestiaCore
  -> initSimulation
  -> SDL/OpenGL AppWindow
  -> historical Renderer / TextureManager / GeometryManager pipeline
```

同一个 exe 也支持 runtime 参数进入多进程路径，但那是另一条验证对象。不要把“统一 exe 还能启动 runtime”与“普通 in-process 主路径能力保真”混为一个结论。

## 3. 固定基线

固定改造前基线为：

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

该提交是：

```text
14062ca refactor: decouple Celestia MVC boundaries
```

的父提交，即本轮 MVC 代码级解耦开始前的本地 Celestia 能力基线。

## 4. 当前机测证据

| 项 | 值 |
| --- | --- |
| 机测模式 | `Full` |
| 机测时间 | `2026-06-27-155813` |
| 当前提交 | `c6d7e53e29035f81d70d84148d93816e2778364b` |
| 基线提交 | `44ec265659d2aa666cbf7546e36e4dde471d54ba` |
| 机测报告 | `DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-06-27-155813-Celestia-compat-regression-machine-report.md` |
| 外部产物 | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-27-155813-c6d7e53-full` |
| 总状态 | `pass` |

## 5. 视觉场景结果

| 场景 | 状态 | dHash Hamming | Average Color Distance |
| --- | --- | --- | --- |
| `01-earth-default` | `pass` | `0` | `0.001` |
| `02-earth-clouds-orbits-labels` | `pass` | `0` | `0.0` |
| `03-moon-close` | `pass` | `0` | `0.0` |
| `04-saturn-rings` | `pass` | `0` | `0.0` |
| `05-asteroid-or-spacecraft` | `pass` | `0` | `0.0` |
| `06-starfield-constellations` | `pass` | `1` | `0.002` |
| `07-galaxy-deepsky` | `pass` | `0` | `0.0` |
| `08-script-overlay-hud` | `pass` | `0` | `0.0` |

这些场景覆盖默认启动、地球、云层、轨道线、标签、月球、土星环、小天体或航天器、星空、星座线、深空对象、脚本 overlay / HUD 等主路径可见能力。

## 6. Runtime smoke 结果

Full 回归同时运行了当前版本的 runtime smoke：

| 配置 | 状态 |
| --- | --- |
| `runtime-2d-stdio.yaml` | `pass` |
| `runtime-3d-stdio.yaml` | `pass` |
| `runtime-2d-local-socket.yaml` | `pass` |
| `runtime-3d-local-socket.yaml` | `pass` |
| `runtime-switch-2d-to-3d-local-socket.yaml` | `pass` |
| `runtime-switch-3d-to-2d-local-socket.yaml` | `pass` |

这些 smoke 证明当前 runtime 路径仍可启动、通信、切换和退出。它们是对 MVC runtime 的补充验证，不是原 Celestia 视觉能力保真的替代证据。

## 7. 允许表述

可以表述：

```text
当前 master 未在自动化覆盖范围内发现普通 SDL 统一 exe / in-process 主路径相对改造前基线的可见退化。
```

可以表述：

```text
当前回归体系已经能把改造前固定基线与当前提交进行多场景截图和 runtime smoke 对照。
```

可以表述：

```text
后续重要修改后应至少跑 Quick，阶段收口或大范围变更后应跑 Full。
```

## 8. 禁止表述

不能表述：

```text
所有 Celestia 功能均已自动验证。
```

不能表述：

```text
当前版本与改造前版本逐像素一致。
```

不能表述：

```text
Qt / Win32 前端能力已经完成同等验证。
```

不能表述：

```text
跨进程 OpenGL3D View 已具备完整历史 Renderer 视觉等价。
```

不能表述：

```text
所有天文数值、catalog、add-on、菜单、URL、脚本、键鼠导航和性能指标均已覆盖。
```

## 9. 后续使用机制

日常修改后：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

阶段验收或大范围修改后：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

只有 `Full` 报告为 `pass` 后，才应更新阶段性保真结论。

如果基线产物缺失或机器环境重建：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
```

`InitBaseline` 仍应固定在 `44ec265659d2aa666cbf7546e36e4dde471d54ba`，除非项目明确决定重定新的历史基线。

## 10. 当前风险与下一步

当前主要剩余风险：

1. 覆盖场景仍是代表性抽样，不是 Celestia 全功能枚举。
2. Qt / Win32 前端没有纳入第一版自动回归。
3. 菜单级交互、完整键鼠导航、URL、add-on、性能、视频捕获和天文数值精度没有纳入第一版门禁。
4. runtime OpenGL3D host 的视觉能力仍弱于普通 in-process 历史 Renderer。

下一步应继续扩展场景矩阵，同时启动真实 Celestia scene projection / View3D visual parity 路线，使多进程 runtime 逐步接近普通 in-process 主路径能力。
