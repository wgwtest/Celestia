# Celestia Compatibility Regression Machine Report

- mode: `Step18`
- timestamp: `2026-06-28-101051`
- current commit: `a03ce27024c617ffd67de41188d7c8e65473b28c`
- baseline commit: `44ec265659d2aa666cbf7546e36e4dde471d54ba`
- artifacts: `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18`
- status: `pass`

## Current Screenshots

| Scene | Status | Image | Metrics |
| --- | --- | --- | --- |
| `01-earth-default` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\01-earth-default.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\01-earth-default.json` |
| `02-earth-clouds-orbits-labels` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\02-earth-clouds-orbits-labels.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\02-earth-clouds-orbits-labels.json` |
| `03-moon-close` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\03-moon-close.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\03-moon-close.json` |
| `04-saturn-rings` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\04-saturn-rings.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\04-saturn-rings.json` |
| `05-asteroid-or-spacecraft` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\05-asteroid-or-spacecraft.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\05-asteroid-or-spacecraft.json` |
| `06-starfield-constellations` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\06-starfield-constellations.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\06-starfield-constellations.json` |
| `07-galaxy-deepsky` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\07-galaxy-deepsky.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\07-galaxy-deepsky.json` |
| `08-script-overlay-hud` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\08-script-overlay-hud.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\08-script-overlay-hud.json` |
| `09-selection-follow-goto` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\09-selection-follow-goto.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\09-selection-follow-goto.json` |
| `10-resource-fallback-missing` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\screenshots\current\10-resource-fallback-missing.png` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\metrics\current\10-resource-fallback-missing.json` |

## Runtime Smoke

| Config | Status | Detail |
| --- | --- | --- |
| `runtime-2d-stdio.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-2d-stdio.yaml` |
| `runtime-3d-stdio.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-3d-stdio.yaml` |
| `runtime-2d-local-socket.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-2d-local-socket.yaml` |
| `runtime-3d-local-socket.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-3d-local-socket.yaml` |
| `runtime-switch-2d-to-3d-local-socket.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-switch-2d-to-3d-local-socket.yaml` |
| `runtime-switch-3d-to-2d-local-socket.yaml` | `pass` | `D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101051-a03ce27-step18\logs\runtime-smoke\runtime-switch-3d-to-2d-local-socket.yaml` |

## Step18 Claim Boundary

A passing `Step18` run supports only this claim:

```text
Step18 found no failure in the current SDL screenshot matrix or in the strengthened multi-process View3D runtime smoke checks covered by this report.
```

It does not prove complete historical renderer parity, exhaustive Celestia feature parity, pixel-perfect rendering, or Qt/Win32 frontend parity.
