# Celestia Step12 SceneFrame Contract Machine Report

- timestamp: `2026-06-27-172715`
- branch: `codex/celestia-mvc-step12-scene-frame`
- status: `checkpoint-pass`
- scope: `scene.frame vNext contract, validation helper, fixtures, Step8 compatibility`

## Changed Contract Surface

```text
src/celruntime/protocol/sceneprotocol.h
src/celruntime/protocol/sceneprotocol.cpp
src/celruntime/model/sceneextractor.cpp
src/celruntime/model/modelservice.cpp
test/unit/mvc_step12_scene_frame_contract_test.cpp
test/fixtures/mvc/*.sceneframe
```

## TDD Red Evidence

The Step12 test file and CMake entry were added before production code changes.

```text
command:
cmd /s /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul && "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build-mvc-sdl-rel --config Release --target unit -- -k 0'

expected failure:
test/unit/mvc_step12_scene_frame_contract_test.cpp failed to compile because SceneFrameProtocolVersion, protocolVersion, TimeState, ResourceRef.id, LabelRenderState, DeepSkyRenderState, validateSceneFrame, and isValidSceneFrame did not exist yet.
```

An earlier build attempt without the VS developer environment failed before reaching Step12 tests because MSVC standard library and Windows SDK paths were absent from the PowerShell environment. The reproducible RED command above used `vcvars64.bat`.

## Build And Test Evidence

| Gate | Result |
| --- | --- |
| SDL `unit` build | passed |
| SDL Step8/Step12 focused CTest | passed, `11/11` |
| SDL full CTest | passed, `166/166` |
| baseline `unit` build | passed |
| baseline Step8/Step12 focused CTest | passed, `11/11` |
| baseline full CTest | passed, `166/166` |
| MVC dependency scan | passed |
| MVC CMake target scan | passed |

Focused command:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release -R "Step8|Step12|SceneFrame|SceneProtocol" --output-on-failure
```

Full SDL command:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build-mvc-sdl-rel -C Release --output-on-failure
```

Scans:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_mvc_dependencies.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\mvc\scan_cmake_targets.ps1
```

## Claim Boundary

This checkpoint supports:

```text
scene.frame vNext has a protocol-level contract with DTO fields, flat serializer compatibility, fixtures, and validation tests.
Step8 legacy scene.frame payloads still deserialize.
The protocol header remains renderer/OpenGL free.
```

This checkpoint does not support:

```text
real Celestia Universe is running inside the independent Model host.
cross-process View3D has historical renderer visual parity.
all real catalog, mesh, texture, orbit, and label resources use the final data-plane.
```

## Remaining Before Step12 Closure

```text
Create checkpoint commit.
Run Full compatibility regression after the checkpoint commit so the screenshot report records the tested commit.
Inspect baseline/current screenshot comparison output.
Update this record or add the generated Full report path.
```
