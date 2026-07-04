$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$scriptPath = Join-Path $repoRoot "tools\mvc\scan_mvc_boundary_debt.ps1"
if (-not (Test-Path $scriptPath)) {
    throw "missing MVC boundary debt scanner: ${scriptPath}"
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("celestia-mvc-boundary-test-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null

try {
    $modelDir = Join-Path $tempRoot "src\celengine\model"
    $controllerDir = Join-Path $tempRoot "src\celengine\controller"
    $adapterDir = Join-Path $tempRoot "src\celengine\adapter"
    $runtimeModelDir = Join-Path $tempRoot "src\celruntime\model"
    New-Item -ItemType Directory -Path $modelDir, $controllerDir, $adapterDir, $runtimeModelDir | Out-Null

    Set-Content -LiteralPath (Join-Path $modelDir "body.cpp") -Value @'
#include <celengine/view3d/referencemark.h>
#include <celengine/adapter/solarsys.h>
void f() {}
'@ -Encoding UTF8

    Set-Content -LiteralPath (Join-Path $controllerDir "simulation.h") -Value @'
#include <celengine/view3d/texture.h>
class Simulation {};
'@ -Encoding UTF8

    Set-Content -LiteralPath (Join-Path $adapterDir "sceneviewmodel.h") -Value @'
#include <celruntime/viewframe.h>
class SceneViewModel {};
'@ -Encoding UTF8

    Set-Content -LiteralPath (Join-Path $runtimeModelDir "realmodelbackend.cpp") -Value @'
#include <celengine/view3d/meshmanager.h>
#include <celestia/loadstars.h>
void f() {}
'@ -Encoding UTF8

    $reportOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath -RepoRoot $tempRoot 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "report mode should return 0, got $LASTEXITCODE`n$($reportOutput -join "`n")"
    }

    $reportText = $reportOutput -join "`n"
    foreach ($expected in @(
        "model->view",
        "model->adapter",
        "controller->view",
        "adapter->runtime",
        "runtime-model->view",
        "runtime-model->app"
    )) {
        if ($reportText -notmatch [regex]::Escape($expected)) {
            throw "report mode did not include ${expected}`n${reportText}"
        }
    }

    $failOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath -RepoRoot $tempRoot -FailOnFindings 2>&1
    if ($LASTEXITCODE -eq 0) {
        throw "FailOnFindings should return non-zero when findings exist`n$($failOutput -join "`n")"
    }

    "MVC boundary debt scan self-test passed"
}
finally {
    if (Test-Path $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
