# Celestia Step18 Cross-Process View3D Fidelity Acceptance Outline

## Scope

This outline validates Step18 evidence only. It does not declare complete historical renderer parity.

## Required Machine Evidence

```text
Step18 regression mode passes.
Current SDL screenshot matrix has at least 10 scenarios and passes.
The matrix includes selection/follow/goto and resource fallback coverage scenes.
Multi-process View3D runtime smoke logs contain view.frameRendered count and payload.
Runtime smoke logs contain bodyCount and resourceCount for 3D configs.
No residual Celestia runtime processes remain after the run.
MVC dependency scan passes.
MVC CMake target scan passes.
UTF-8 BOM runtime config files preserve session fields.
Regression process helper drains redirected stdout and stderr asynchronously.
```

## Claim Boundary

Can say:

```text
Step18 has a machine-verifiable evidence gate for current SDL visuals and multi-process View3D real-scene runtime output.
```

Cannot say:

```text
Cross-process View3D has complete historical renderer visual parity.
Qt or Win32 frontend parity has been proven.
Every Celestia visual feature is covered.
```
