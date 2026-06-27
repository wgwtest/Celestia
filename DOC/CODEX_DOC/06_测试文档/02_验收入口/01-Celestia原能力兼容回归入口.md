# Celestia 原能力兼容回归入口

日期：2026-06-27

## 1. 用途

本入口用于验证：

```text
MVC 解耦和多进程 runtime 演进后，普通 SDL 统一 exe / in-process 主路径没有丢失改造前 Celestia 的主要可见能力。
```

它不是新的多进程 View3D 视觉等价验收，也不是 Qt / Win32 前端验收。

## 2. 固定基线

固定基线提交：

```text
44ec265659d2aa666cbf7546e36e4dde471d54ba
```

该提交是：

```text
14062ca refactor: decouple Celestia MVC boundaries
```

的父提交，即本轮 MVC 代码级解耦开始前的本地 Celestia 能力基线。

## 3. 验收命令

脚本自检，不构建、不启动 Celestia：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode SelfTest
```

当前版本快速门禁：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

初始化或刷新固定基线截图：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode InitBaseline
```

完整 baseline/current 对比：

```powershell
powershell -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Full
```

## 4. 覆盖场景

第一版包含 8 个 CEL 场景：

```text
01-earth-default
02-earth-clouds-orbits-labels
03-moon-close
04-saturn-rings
05-asteroid-or-spacecraft
06-starfield-constellations
07-galaxy-deepsky
08-script-overlay-hud
```

这些场景覆盖基础行星渲染、云层、轨道、标签、月球、土星环、小天体、恒星/星座、深空对象、脚本叠字和 HUD。

## 5. 产物位置

默认产物目录：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\
```

其中：

```text
baselines\44ec265\
runs\<timestamp>-<commit>-<mode>\
```

`Full` 模式还会把机测报告复制到：

```text
DOC\CODEX_DOC\06_测试文档\03_机测记录\
```

## 6. 可允许的结论

只有 `Full` 通过后，才允许写入阶段性结论：

```text
本阶段未发现普通 SDL 统一 exe / in-process 主路径相对 MVC 改造前基线的可见能力退化。
```

不能表述为：

```text
所有 Celestia 功能均已自动验证。
所有视觉效果与原版逐像素一致。
Qt / Win32 前端也已完成同等验证。
```
