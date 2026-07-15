param(
    [ValidateSet("pass", "warn", "fail", "error")]
    [string] $Status = "pass",

    [Parameter(Mandatory = $true)]
    [string] $OutputRoot,

    [string] $ChecksPath
)

$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "CelestiaCompatRegression.psm1") -Force

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$runId = if ([string]::IsNullOrWhiteSpace($ChecksPath)) { "probe-$Status" } else { Split-Path -Leaf $OutputRoot }
try {
    if ([string]::IsNullOrWhiteSpace($ChecksPath)) {
        $checks = @(
            New-RegressionCheck `
                -Id "probe/$Status" `
                -Group "probe" `
                -Status $Status `
                -Required $true `
                -Detail "gate probe"
        )
    }
    else {
        if (-not (Test-Path -LiteralPath $ChecksPath -PathType Leaf)) {
            throw "checks JSON is missing: $ChecksPath"
        }
        $checksJson = Get-Content -LiteralPath $ChecksPath -Raw -Encoding UTF8
        if ([string]::IsNullOrWhiteSpace($checksJson)) {
            throw "checks JSON is empty: $ChecksPath"
        }
        try {
            $checksDocument = $checksJson | ConvertFrom-Json
        }
        catch {
            throw "checks JSON is invalid: $($_.Exception.Message)"
        }
        if ($null -ne $checksDocument.PSObject.Properties["checks"]) {
            $checks = @($checksDocument.checks)
        }
        else {
            $checks = @($checksDocument)
        }
        if ($checks.Count -eq 0) {
            throw "checks JSON contains no checks: $ChecksPath"
        }
    }
}
catch {
    $checks = @(
        New-RegressionCheck `
            -Id "harness/gate-probe-input" `
            -Group "harness" `
            -Status error `
            -Required $true `
            -Detail $_.Exception.Message `
            -Evidence @($ChecksPath)
    )
}

try {
    $summary = New-RegressionRunSummary `
        -RunId $runId `
        -Mode $(if ([string]::IsNullOrWhiteSpace($ChecksPath)) { "Probe" } else { "FaultInjection" }) `
        -CurrentCommit "probe" `
        -BaselineCommit "probe" `
        -VerificationMatrix "probe" `
        -ArtifactRoot $OutputRoot `
        -Checks $checks
    Write-RegressionReports -Summary $summary -OutputRoot $OutputRoot | Out-Null
    exit $summary.exitCode
}
catch {
    Write-Error $_
    exit 3
}
