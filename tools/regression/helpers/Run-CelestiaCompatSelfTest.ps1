$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$regressionRoot = Resolve-Path (Join-Path $scriptRoot "..")
$repoRoot = Resolve-Path (Join-Path $regressionRoot "..\..")

function Assert-Exists {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $Message
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw $Message
    }
}

Assert-Exists (Join-Path $regressionRoot "run_celestia_compat_regression.ps1") "Missing regression runner"
Assert-Exists (Join-Path $scriptRoot "image_metrics.py") "Missing image metrics helper"

$runnerText = Get-Content -LiteralPath (Join-Path $regressionRoot "run_celestia_compat_regression.ps1") -Raw
if ($runnerText -notmatch 'ValidateSet\("SelfTest", "InitBaseline", "Quick", "Full", "Step18"\)') {
    throw "Regression runner does not expose Step18 mode"
}
foreach ($required in @("Invoke-Step18", "Copy-Step18ReportToDocs", "Get-RuntimeSmokeStatus")) {
    if ($runnerText -notmatch $required) {
        throw "Regression runner is missing Step18 helper: $required"
    }
}
foreach ($required in @("ReadToEndAsync")) {
    if ($runnerText -notmatch $required) {
        throw "Regression runner does not drain redirected process output asynchronously: $required"
    }
}
foreach ($required in @("Step18 Claim Boundary", "does not prove complete historical renderer parity")) {
    if ($runnerText -notmatch [regex]::Escape($required)) {
        throw "Step18 report boundary text is missing: $required"
    }
}

$scenarioRoot = Join-Path $regressionRoot "scenarios"
Assert-Exists $scenarioRoot "Missing scenario directory"

$scenarios = @(Get-ChildItem -LiteralPath $scenarioRoot -Filter "*.cel" -File | Sort-Object Name)
if ($scenarios.Count -lt 10) {
    throw "Expected at least 10 CEL scenarios for Step18, found $($scenarios.Count)"
}

foreach ($scenario in $scenarios) {
    $text = Get-Content -LiteralPath $scenario.FullName -Raw
    if ($text -notmatch '\$CAPTURE_FILE\$') {
        throw "Scenario $($scenario.Name) does not contain capture placeholder"
    }
    if ($text -notmatch 'exit\s*\{\s*\}') {
        throw "Scenario $($scenario.Name) does not exit after capture"
    }
}

$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    throw "python command was not found"
}

& $python.Source (Join-Path $scriptRoot "image_metrics.py") --self-test
if ($LASTEXITCODE -ne 0) {
    throw "image metrics helper self-test failed"
}

Write-Output "Celestia compatibility regression self-test passed"
