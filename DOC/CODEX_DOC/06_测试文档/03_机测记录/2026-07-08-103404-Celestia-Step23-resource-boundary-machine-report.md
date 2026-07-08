# Celestia Step23 资源索引边界机测记录

## 1. 测试对象

本次机测覆盖 Step23 “资源引用与资源索引边界”代码落地。

本轮目标不是完成 Model 层整体解耦，而是验证以下窄目标：

1. `TexturePaths` / `GeometryPaths` 已从 View3D manager 头文件中剥离到中立资源层。
2. `RealModelBackend` 不再直接 include View3D 资源管理器头文件。
3. 统一 SDL exe、headless model backend、runtime smoke 和截图回归没有因本次资源索引迁移出现可见退化。

回归脚本报告记录的执行时 HEAD：

```text
c4dc50dc8474dd0ea7fe38bb0bec11e8a012cd13
```

说明：脚本报告中的 `current commit` 来自执行时 `git rev-parse HEAD`；回归命令实际构建的是执行时工作区内容，其中包含 Step23 本轮未提交代码差异。后续提交只改变 Git 历史定位，不改变本轮机测所覆盖的源码内容。

## 2. 构建验证

构建命令：

```powershell
cmake --build build-mvc-sdl-rel --config Release --target unit celestia-model-host celestia-sdl
```

结果：

```text
pass
```

补充说明：

1. `unit` 链接成功。
2. `celestia-model-host` 构建成功。
3. `celestia-sdl` 构建成功。
4. 构建过程中出现 CMake runtime dependency 复制相关开发警告，不影响目标构建结果。

## 3. 单测验证

Step23 专用测试：

```powershell
ctest --test-dir build-mvc-sdl-rel -C Release -I 86,89 --output-on-failure
```

结果：

```text
4/4 tests passed
```

通过的 Step23 测试：

| 编号 | 测试名 | 结果 |
|---:|---|---|
| 86 | `resource path indexes are declared outside View3D` | pass |
| 87 | `View3D managers include resource indexes instead of owning them` | pass |
| 88 | `RealModelBackend does not include View3D resource manager headers` | pass |
| 89 | `resource object library is wired into unified and headless builds` | pass |

相关 MVC 回归测试：

```powershell
ctest --test-dir build-mvc-sdl-rel -C Release -R "RuntimeAssemblyConfig|RuntimeAssemblyRunner|Step13|Step17|resource path indexes|View3D managers include resource|RealModelBackend does not include View3D|resource object library|celengine CMake|CMake defines physical MVC targets|CMake declares MVC target dependency direction|SDL launcher routes multi-process|RuntimeSession routes messages|RuntimeSession sustains|RuntimeSession can stop" --output-on-failure
```

结果：

```text
32/32 tests passed
```

覆盖范围：

1. Step13 真实 Model 后端数据加载。
2. Step17 资源引用稳定性。
3. Step23 资源索引边界。
4. MVC CMake 目标装配。
5. runtime session 进程通信、切换和 OpenGL3D/debug2D 加载路径。

## 4. 边界扫描

命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_boundary_debt.ps1
```

结果摘要：

```text
MVC boundary debt scan report: 53 finding(s)
```

| 扫描类别 | 当前数量 | Step23 判断 |
|---|---:|---|
| `runtime-model->view` | 0 | 本轮目标已清零 |
| `adapter->view` | 4 | 剩余为真实 Adapter/View 依赖，后续步骤处理 |
| `runtime-model->app` | 4 | 仍为真实数据加载入口问题，后续步骤处理 |
| `runtime-model-projection` | 44 | 运行时输出仍是过渡投影结构，后续步骤处理 |
| `adapter->runtime` | 1 | `SceneViewModel` 与 runtime 输出边界问题，后续步骤处理 |

命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File test\scripts\test_mvc_model_adapter_boundary_clean.ps1
```

结果：

```text
MVC model adapter boundary is clean
```

## 5. Quick 兼容回归

命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\regression\run_celestia_compat_regression.ps1 -Mode Quick
```

结果：

```text
status: pass
```

回归报告：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-08-103404-c4dc50d-quick\machine-report.md
```

Quick 回归自动执行内容：

1. `unit` 和 `celestia-sdl` 构建。
2. 全量 CTest。
3. MVC 依赖扫描。
4. CMake target 扫描。
5. Model/Adapter 边界脚本。
6. 6 个 runtime smoke 配置。
7. 10 个统一 SDL exe 截图场景。

Runtime smoke 结果：

| 配置 | 结果 |
|---|---|
| `runtime-2d-stdio.yaml` | pass |
| `runtime-3d-stdio.yaml` | pass |
| `runtime-2d-local-socket.yaml` | pass |
| `runtime-3d-local-socket.yaml` | pass |
| `runtime-switch-2d-to-3d-local-socket.yaml` | pass |
| `runtime-switch-3d-to-2d-local-socket.yaml` | pass |

## 6. 截图结果

截图目录：

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-07-08-103404-c4dc50d-quick\screenshots\current
```

| 场景 | 尺寸 | 非黑像素比例 | 结果 |
|---|---:|---:|---|
| `01-earth-default` | 2560x1377 | 0.018088 | pass |
| `02-earth-clouds-orbits-labels` | 2560x1377 | 0.029985 | pass |
| `03-moon-close` | 2560x1377 | 0.161925 | pass |
| `04-saturn-rings` | 2560x1377 | 0.073415 | pass |
| `05-asteroid-or-spacecraft` | 2560x1377 | 0.018747 | pass |
| `06-starfield-constellations` | 2560x1377 | 0.029567 | pass |
| `07-galaxy-deepsky` | 2560x1377 | 0.015519 | pass |
| `08-script-overlay-hud` | 2560x1377 | 0.019500 | pass |
| `09-selection-follow-goto` | 2560x1377 | 0.031478 | pass |
| `10-resource-fallback-missing` | 2560x1377 | 0.030303 | pass |

## 7. 结论

Step23 本轮机测通过。可以确认：

1. `RealModelBackend -> View3D manager header` 的直接资源路径依赖已清零。
2. 中立资源索引层已经纳入 CMake 构建和单测边界检查。
3. 统一 SDL exe 的 Quick 兼容回归和 runtime smoke 通过。

不能据此扩大为以下结论：

1. Model 层已经整体解耦完成。
2. Builder 家族已经拆分完成。
3. `Surface` / `Atmosphere` 资源字段已经完成重构。
4. 运行时输出已经完成最终分层。
5. View3D 已经完成模板化或平权迁移。
