# Celestia Cross-Process View3D Fidelity Stage Conclusion

> 2026-07-15 calibration: this document records the Step18 result at that time. The later Step25 audit established that the Runtime 3D payload is synthetic, not original Celestia real-scene output. Scenario names such as `resource-fallback-missing` do not prove the named failure behavior unless a matching checkpoint exists.

## Status

Step18 is complete for the current evidence-gate scope. It establishes a machine-verifiable check path for the current SDL screenshot matrix and multi-process View3D runtime-smoke output.

Latest machine report:

```text
DOC/CODEX_DOC/06_测试文档/03_机测记录/2026-06-28-101621-Celestia-Step18-machine-report.md
```

Latest artifact reports:

```text
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101621-7b81fca-step18\machine-report.md
D:\WorkSpace\Codex\CeleNew\.regression-artifacts\Celestia\runs\2026-06-28-101820-7b81fca-quick\machine-report.md
```

## Covered Evidence

```text
Step18 regression mode passes.
Current SDL screenshot matrix passes with 10 scenarios.
The matrix includes the named selection/follow/goto and resource fallback scenes; the final images do not prove persistent follow or an injected missing-resource fallback.
Multi-process View3D runtime smoke logs contain view.frameRendered count and payload.
Runtime smoke logs contain bodyCount and resourceCount for 3D configs.
UTF-8 BOM runtime config loading is covered by unit test.
Redirected process stdout/stderr async draining is covered by self-test and Step18 runtime output.
Step12-17 focused CTest gate passes with 50/50 tests.
Release build of celestia-sdl passes.
Quick regression passes after the final Step18 run.
MVC dependency and CMake target scans pass.
```

## Claim Boundary

Can say:

```text
Step18 found no failure in the current SDL screenshot matrix or in the strengthened multi-process View3D runtime smoke checks covered by the latest report.
```

Cannot say:

```text
Cross-process View3D has complete historical renderer visual parity.
Qt or Win32 frontend parity has been proven.
Every Celestia visual feature is covered.
```

## Final Step18 Boundary

```text
Task 3 visual scenario expansion is complete.
Task 4 final verification is complete.
The remaining work is a later MVC phase for deeper cross-process View3D implementation, not additional Step18 evidence-gate work.
```
