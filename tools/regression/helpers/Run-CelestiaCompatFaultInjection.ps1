param(
    [string] $HelperRoot = $PSScriptRoot
)

$ErrorActionPreference = "Stop"

$helperRootPath = (Resolve-Path $HelperRoot).Path
$regressionRoot = (Resolve-Path (Join-Path $helperRootPath "..")).Path
$repoRoot = (Resolve-Path (Join-Path $regressionRoot "..\..")).Path
$modulePath = Join-Path $helperRootPath "CelestiaCompatRegression.psm1"
$gateProbe = Join-Path $helperRootPath "Invoke-CelestiaCompatGateProbe.ps1"
$imageMetrics = Join-Path $helperRootPath "image_metrics.py"
$verificationMatrixPath = Join-Path $regressionRoot "verification-matrix.json"
$scenarioRoot = Join-Path $regressionRoot "scenarios"
$python = (Get-Command python -ErrorAction Stop).Source
$faultRoot = Join-Path $env:TEMP "CelestiaCompatFaultInjection"

Import-Module $modulePath -Force

function Reset-FaultDirectory {
    param([Parameter(Mandatory = $true)][string] $Path)

    $resolved = [System.IO.Path]::GetFullPath($Path)
    $allowed = [System.IO.Path]::GetFullPath($faultRoot).TrimEnd("\") + "\"
    if (-not $resolved.StartsWith($allowed, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to reset path outside fault fixture root: $resolved"
    }
    if (Test-Path -LiteralPath $resolved) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    New-Item -ItemType Directory -Path $resolved -Force | Out-Null
}

function New-PreparedChecks {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $Checks,
        [AllowNull()][string] $RawJson
    )

    return [pscustomobject]@{
        Checks = @($Checks)
        RawJson = $RawJson
    }
}

function New-FixtureImage {
    param(
        [Parameter(Mandatory = $true)][string] $Path,
        [Parameter(Mandatory = $true)][string] $Color
    )

    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    $code = "from PIL import Image; import sys; c=tuple(int(x) for x in sys.argv[2].split(',')); Image.new('RGB',(160,120),c).save(sys.argv[1])"
    & $python -c $code $Path $Color
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Pillow fixture creation failed: $Path"
    }
}

function New-BaselineFixture {
    param([Parameter(Mandatory = $true)][string] $Root)

    New-Item -ItemType Directory -Path (Join-Path $Root "screenshots\baseline") -Force | Out-Null
    $entries = @(
        [pscustomobject]@{ scenarioId = "scene-a"; checkpointId = "final"; relativePath = "screenshots/baseline/a.png" },
        [pscustomobject]@{ scenarioId = "scene-b"; checkpointId = "final"; relativePath = "screenshots/baseline/b.png" },
        [pscustomobject]@{ scenarioId = "scene-c"; checkpointId = "t1"; relativePath = "screenshots/baseline/c-t1.png" }
    )
    Set-Content -LiteralPath (Join-Path $Root "screenshots\baseline\a.png") -Value "fixture-a" -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $Root "screenshots\baseline\b.png") -Value "fixture-b" -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $Root "screenshots\baseline\c-t1.png") -Value "fixture-c" -Encoding UTF8
    $manifestPath = Join-Path $Root "baseline-manifest.json"
    New-BaselineManifest -BaselineCommit "fixture-commit" -BaselineRoot $Root -ImageEntries $entries -OutputPath $manifestPath | Out-Null
    return [pscustomobject]@{
        Entries = $entries
        ManifestPath = $manifestPath
    }
}

function Invoke-GateForPreparedChecks {
    param(
        [Parameter(Mandatory = $true)][string] $CaseId,
        [Parameter(Mandatory = $true)][string] $Variant,
        [Parameter(Mandatory = $true)][string] $CaseRoot,
        [Parameter(Mandatory = $true)][object] $Prepared,
        [Parameter(Mandatory = $true)][string] $ExpectedStatus,
        [Parameter(Mandatory = $true)][int] $ExpectedExitCode,
        [string[]] $ExpectedEvidence = @()
    )

    $checksPath = Join-Path $CaseRoot ($Variant + "-checks.json")
    if (-not [string]::IsNullOrEmpty([string]$Prepared.RawJson)) {
        Set-Content -LiteralPath $checksPath -Value $Prepared.RawJson -Encoding UTF8
    }
    else {
        [pscustomobject]@{ checks = @($Prepared.Checks) } | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $checksPath -Encoding UTF8
    }

    $outputRoot = Join-Path $CaseRoot ($Variant + "-report")
    New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
    $stdoutPath = Join-Path $CaseRoot ($Variant + ".stdout.log")
    $stderrPath = Join-Path $CaseRoot ($Variant + ".stderr.log")
    $process = Start-Process `
        -FilePath "powershell" `
        -ArgumentList @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-File", $gateProbe,
            "-OutputRoot", $outputRoot,
            "-ChecksPath", $checksPath
        ) `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath `
        -Wait `
        -PassThru

    $jsonPath = Join-Path $outputRoot "machine-report.json"
    $markdownPath = Join-Path $outputRoot "machine-report.md"
    if (-not (Test-Path -LiteralPath $jsonPath -PathType Leaf) -or -not (Test-Path -LiteralPath $markdownPath -PathType Leaf)) {
        throw "$CaseId expected/actual mismatch: gate reports are missing; stdout=$stdoutPath stderr=$stderrPath"
    }
    $summary = Get-Content -LiteralPath $jsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $markdown = Get-Content -LiteralPath $markdownPath -Raw -Encoding UTF8
    if ($process.ExitCode -ne $ExpectedExitCode -or [string]$summary.status -ne $ExpectedStatus -or [int]$summary.exitCode -ne $ExpectedExitCode) {
        throw "$CaseId expected/actual mismatch: expected=$ExpectedStatus/$ExpectedExitCode actual=$($summary.status)/$($process.ExitCode) reportExit=$($summary.exitCode)"
    }
    foreach ($expectedLine in @(
        "| Status | $ExpectedStatus |",
        "| Exit code | $ExpectedExitCode |"
    )) {
        if ($markdown -notmatch [regex]::Escape($expectedLine)) {
            throw "$CaseId expected/actual mismatch: Markdown is missing [$expectedLine]"
        }
    }
    $combinedEvidence = (Get-Content -LiteralPath $jsonPath -Raw -Encoding UTF8) + "`n" + (Get-Content -LiteralPath $stderrPath -Raw -Encoding UTF8)
    foreach ($needle in $ExpectedEvidence) {
        if ($combinedEvidence -notmatch [regex]::Escape($needle)) {
            throw "$CaseId expected/actual mismatch: evidence is missing [$needle]"
        }
    }

    return [pscustomobject]@{
        Id = $CaseId
        Variant = $Variant
        Status = [string]$summary.status
        ExitCode = [int]$process.ExitCode
        Json = $jsonPath
        Markdown = $markdownPath
    }
}

function Invoke-FaultCase {
    param([Parameter(Mandatory = $true)][hashtable] $Case)

    $caseRoot = Join-Path $faultRoot $Case.Id
    Reset-FaultDirectory -Path $caseRoot
    $faultFixture = Join-Path $caseRoot "fixture-fault"
    New-Item -ItemType Directory -Path $faultFixture -Force | Out-Null
    $faultPrepared = & $Case.Prepare $faultFixture $false
    $faultResult = Invoke-GateForPreparedChecks `
        -CaseId $Case.Id `
        -Variant "fault" `
        -CaseRoot $caseRoot `
        -Prepared $faultPrepared `
        -ExpectedStatus $Case.ExpectedStatus `
        -ExpectedExitCode $Case.ExpectedExitCode `
        -ExpectedEvidence @($Case.ExpectedEvidence)

    if ($Case.Id -ne "FI-00") {
        $cleanFixture = Join-Path $caseRoot "fixture-clean"
        New-Item -ItemType Directory -Path $cleanFixture -Force | Out-Null
        $cleanPrepared = & $Case.Prepare $cleanFixture $true
        Invoke-GateForPreparedChecks `
            -CaseId $Case.Id `
            -Variant "clean" `
            -CaseRoot $caseRoot `
            -Prepared $cleanPrepared `
            -ExpectedStatus "pass" `
            -ExpectedExitCode 0 | Out-Null
    }
    return $faultResult
}

$cases = @(
    @{
        Id = "FI-00"; ExpectedStatus = "pass"; ExpectedExitCode = 0; ExpectedEvidence = @("positive fixture")
        Prepare = {
            param($root, $clean)
            New-PreparedChecks -Checks @(
                (New-RegressionCheck -Id "positive/matrix" -Group "fixture" -Status pass -Required $true -Detail "positive fixture matrix"),
                (New-RegressionCheck -Id "positive/baseline" -Group "fixture" -Status pass -Required $true -Detail "positive fixture baseline"),
                (New-RegressionCheck -Id "positive/runtime" -Group "fixture" -Status pass -Required $true -Detail "positive fixture runtime")
            )
        }
    },
    @{
        Id = "FI-01"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("missing scenario id", "10-resource-fallback-missing")
        Prepare = {
            param($root, $clean)
            $matrix = Read-VerificationMatrix -Path $verificationMatrixPath
            $matrixCopyPath = Join-Path $root "verification-matrix.json"
            $matrixCopy = $matrix | ConvertTo-Json -Depth 12 | ConvertFrom-Json
            if (-not $clean) {
                $matrixCopy.unifiedExeScenarios = @($matrixCopy.unifiedExeScenarios | Where-Object id -ne "10-resource-fallback-missing")
            }
            $matrixCopy | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $matrixCopyPath -Encoding UTF8
            $actualMatrix = Read-VerificationMatrix -Path $matrixCopyPath
            try {
                Test-ArtifactSet -Expected @($matrix.unifiedExeScenarios.id) -Actual @($actualMatrix.unifiedExeScenarios.id) -MissingLabel "missing scenario id" -UnexpectedLabel "unexpected scenario id" | Out-Null
                $check = New-RegressionCheck -Id "matrix/scenario-id-set" -Group "matrix" -Status pass -Required $true -Detail "scenario id set matches" -Evidence @($matrixCopyPath)
            }
            catch {
                $check = New-RegressionCheck -Id "matrix/scenario-id-set" -Group "matrix" -Status fail -Required $true -Detail $_.Exception.Message -Evidence @($matrixCopyPath)
            }
            New-PreparedChecks -Checks @($check)
        }
    },
    @{
        Id = "FI-02"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("unexpected scenario script", "unexpected.cel")
        Prepare = {
            param($root, $clean)
            $copyRoot = Join-Path $root "scenarios"
            New-Item -ItemType Directory -Path $copyRoot -Force | Out-Null
            Copy-Item -Path (Join-Path $scenarioRoot "*.cel") -Destination $copyRoot -Force
            if (-not $clean) {
                Set-Content -LiteralPath (Join-Path $copyRoot "unexpected.cel") -Value 'print { text "unexpected" }' -Encoding UTF8
            }
            $matrix = Read-VerificationMatrix -Path $verificationMatrixPath
            $expected = @($matrix.unifiedExeScenarios | ForEach-Object { [System.IO.Path]::GetFileName([string]$_.script) })
            $actual = @(Get-ChildItem -LiteralPath $copyRoot -Filter "*.cel" -File | ForEach-Object Name)
            try {
                Test-ArtifactSet -Expected $expected -Actual $actual -MissingLabel "missing scenario script" -UnexpectedLabel "unexpected scenario script" | Out-Null
                $check = New-RegressionCheck -Id "matrix/scenario-script-set" -Group "matrix" -Status pass -Required $true -Detail "scenario script set matches" -Evidence @($copyRoot)
            }
            catch {
                $check = New-RegressionCheck -Id "matrix/scenario-script-set" -Group "matrix" -Status fail -Required $true -Detail $_.Exception.Message -Evidence @($copyRoot)
            }
            New-PreparedChecks -Checks @($check)
        }
    },
    @{
        Id = "FI-03"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("missing baseline image", "scene-b/final")
        Prepare = {
            param($root, $clean)
            $fixture = New-BaselineFixture -Root $root
            if (-not $clean) {
                Remove-Item -LiteralPath (Join-Path $root "screenshots\baseline\b.png") -Force
            }
            $checks = @(Test-BaselineManifest -ManifestPath $fixture.ManifestPath -BaselineCommit "fixture-commit" -BaselineRoot $root -ExpectedImages $fixture.Entries)
            New-PreparedChecks -Checks $checks
        }
    },
    @{
        Id = "FI-04"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("baseline hash mismatch", "expected=", "actual=")
        Prepare = {
            param($root, $clean)
            $fixture = New-BaselineFixture -Root $root
            if (-not $clean) {
                Set-Content -LiteralPath (Join-Path $root "screenshots\baseline\a.png") -Value "fixture-a-modified" -Encoding UTF8
            }
            $checks = @(Test-BaselineManifest -ManifestPath $fixture.ManifestPath -BaselineCommit "fixture-commit" -BaselineRoot $root -ExpectedImages $fixture.Entries)
            New-PreparedChecks -Checks $checks
        }
    },
    @{
        Id = "FI-05"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("current-image", "nonBlackRatio")
        Prepare = {
            param($root, $clean)
            $imagePath = Join-Path $root "current.png"
            New-FixtureImage -Path $imagePath -Color $(if ($clean) { "40,80,160" } else { "0,0,0" })
            $checkpoint = [pscustomobject]@{ id = "current-image"; kind = "image"; artifact = "current.png"; required = $true }
            $artifacts = @{ ScenarioId = "image-fixture"; ArtifactRoot = $root; ImageRoot = $root; MetricsRoot = (Join-Path $root "metrics"); ImageMetricsScript = $imageMetrics; Python = $python }
            New-PreparedChecks -Checks @((Test-RegressionCheckpoint -Checkpoint $checkpoint -Artifacts $artifacts))
        }
    },
    @{
        Id = "FI-06"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("clean-shutdown", "matches=0")
        Prepare = {
            param($root, $clean)
            $stdout = Join-Path $root "stdout.log"
            Set-Content -LiteralPath $stdout -Value $(if ($clean) { "all hosts stopped" } else { "shutdown token removed" }) -Encoding UTF8
            $checkpoint = [pscustomobject]@{ id = "clean-shutdown"; kind = "stdoutRegex"; pattern = "all hosts stopped"; minimumMatches = 1; required = $true }
            $artifacts = @{ ScenarioId = "runtime-fixture"; Stdout = $stdout; ArtifactRoot = $root }
            New-PreparedChecks -Checks @((Test-RegressionCheckpoint -Checkpoint $checkpoint -Artifacts $artifacts))
        }
    },
    @{
        Id = "FI-07"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("positive-frame-count", "matches=0")
        Prepare = {
            param($root, $clean)
            $stdout = Join-Path $root "stdout.log"
            Set-Content -LiteralPath $stdout -Value $(if ($clean) { "view.frameRendered count=24" } else { "view.frameRendered count=0" }) -Encoding UTF8
            $checkpoint = [pscustomobject]@{ id = "positive-frame-count"; kind = "stdoutRegex"; pattern = "view\.frameRendered count=[1-9][0-9]*"; minimumMatches = 1; required = $true }
            $artifacts = @{ ScenarioId = "runtime-fixture"; Stdout = $stdout; ArtifactRoot = $root }
            New-PreparedChecks -Checks @((Test-RegressionCheckpoint -Checkpoint $checkpoint -Artifacts $artifacts))
        }
    },
    @{
        Id = "FI-08"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("child exit code=17")
        Prepare = {
            param($root, $clean)
            $targetExit = if ($clean) { 0 } else { 17 }
            $stage = Invoke-RegressionStage -Id "runtime-fixture/process" -Group "runtime-process" -Action {
                & cmd.exe /d /c "exit $targetExit"
                $childExit = $LASTEXITCODE
                if ($childExit -ne 0) { throw "child exit code=$childExit" }
            }
            New-PreparedChecks -Checks @($stage.Check)
        }
    },
    @{
        Id = "FI-09"; ExpectedStatus = "warn"; ExpectedExitCode = 2; ExpectedEvidence = @("averageColorDistance")
        Prepare = {
            param($root, $clean)
            $baselineImage = Join-Path $root "baseline.png"
            $currentImage = Join-Path $root "current.png"
            New-FixtureImage -Path $baselineImage -Color "40,80,160"
            New-FixtureImage -Path $currentImage -Color $(if ($clean) { "40,80,160" } else { "200,200,40" })
            $comparisonPath = Join-Path $root "comparison.json"
            & $python $imageMetrics --baseline $baselineImage --current $currentImage --json $comparisonPath
            if ($LASTEXITCODE -ne 0) { throw "image comparison fixture failed" }
            $comparison = Get-Content -LiteralPath $comparisonPath -Raw -Encoding UTF8 | ConvertFrom-Json
            $status = if (-not $comparison.comparison.sameDimensions -or $comparison.current.nonBlackRatio -lt 0.002) {
                "fail"
            }
            elseif ($comparison.comparison.dHashHamming -gt 30 -or $comparison.comparison.averageColorDistance -gt 80) {
                "warn"
            }
            else {
                "pass"
            }
            $check = New-RegressionCheck -Id "comparison/warn-threshold" -Group "comparison" -Status $status -Required $true -Detail ("dHashHamming={0}; averageColorDistance={1}" -f $comparison.comparison.dHashHamming, $comparison.comparison.averageColorDistance) -Evidence @($comparisonPath)
            New-PreparedChecks -Checks @($check)
        }
    },
    @{
        Id = "FI-10"; ExpectedStatus = "error"; ExpectedExitCode = 3; ExpectedEvidence = @("harness/gate-probe-input", "checks JSON")
        Prepare = {
            param($root, $clean)
            if ($clean) {
                New-PreparedChecks -Checks @((New-RegressionCheck -Id "harness/clean-input" -Group "harness" -Status pass -Required $true -Detail "valid checks JSON"))
            }
            else {
                New-PreparedChecks -Checks @() -RawJson '{ invalid json'
            }
        }
    },
    @{
        Id = "FI-11"; ExpectedStatus = "fail"; ExpectedExitCode = 1; ExpectedEvidence = @("second-image", "image checkpoint is missing")
        Prepare = {
            param($root, $clean)
            New-FixtureImage -Path (Join-Path $root "first.png") -Color "40,80,160"
            New-FixtureImage -Path (Join-Path $root "second.png") -Color "160,80,40"
            if (-not $clean) {
                Remove-Item -LiteralPath (Join-Path $root "second.png") -Force
            }
            $artifacts = @{ ScenarioId = "runtime-two-image"; ArtifactRoot = $root; ImageRoot = $root; MetricsRoot = (Join-Path $root "metrics"); ImageMetricsScript = $imageMetrics; Python = $python }
            $checkpoints = @(
                [pscustomobject]@{ id = "first-image"; kind = "image"; artifact = "first.png"; required = $true },
                [pscustomobject]@{ id = "second-image"; kind = "image"; artifact = "second.png"; required = $true }
            )
            $checks = @($checkpoints | ForEach-Object { Test-RegressionCheckpoint -Checkpoint $_ -Artifacts $artifacts })
            New-PreparedChecks -Checks $checks
        }
    }
)

New-Item -ItemType Directory -Path $faultRoot -Force | Out-Null
$results = New-Object System.Collections.Generic.List[object]
foreach ($case in $cases) {
    $result = Invoke-FaultCase -Case $case
    $results.Add($result)
    Write-Output ("{0}: expected={1}/{2} actual={3}/{4}" -f $case.Id, $case.ExpectedStatus, $case.ExpectedExitCode, $result.Status, $result.ExitCode)
}

Write-Output ("Celestia compatibility harness fault injection passed: {0}/{1} cases matched expected status and exit code" -f $results.Count, $cases.Count)
