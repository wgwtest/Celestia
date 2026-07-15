# Celestia Step18 Cross-Process View3D Fidelity Acceptance Outline

## Scope

This outline validates Step18 evidence only. It does not declare complete historical renderer parity.

2026-07-15 calibration: Runtime 3D payloads in this scope come from the synthetic model fixture. They are not original Celestia real-scene output.

## Required Machine Evidence

```text
Step18 regression mode passes.
Current SDL screenshot matrix matches verification-matrix.json and passes; the current set contains 10 scenarios.
The matrix includes scenarios named selection-follow-goto and resource-fallback-missing, but their final images do not prove persistent follow or missing-resource fallback.
Multi-process View3D runtime smoke logs contain view.frameRendered count and payload.
Runtime smoke logs contain synthetic bodyCount and resourceCount for 3D configs.
No residual Celestia runtime processes remain after the run.
MVC dependency scan passes.
MVC CMake target scan passes.
UTF-8 BOM runtime config files preserve session fields.
Regression process helper drains redirected stdout and stderr asynchronously.
```

## Claim Boundary

Can say:

```text
Step18 has a machine-verifiable evidence gate for current SDL visuals and multi-process View3D synthetic runtime output.
```

Cannot say:

```text
Cross-process View3D has complete historical renderer visual parity.
Cross-process View3D has produced the original Celestia business scene.
Qt or Win32 frontend parity has been proven.
Every Celestia visual feature is covered.
```
