$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$regressionRoot = Resolve-Path (Join-Path $scriptRoot "..")
$repoRoot = Resolve-Path (Join-Path $regressionRoot "..\..")

Import-Module (Join-Path $scriptRoot "CelestiaCompatRegression.psm1") -Force

function Assert-Equal {
    param(
        [Parameter(Mandatory = $true)] $Actual,
        [Parameter(Mandatory = $true)] $Expected,
        [Parameter(Mandatory = $true)][string] $Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message expected=[$Expected] actual=[$Actual]"
    }
}

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

function Assert-ThrowsLike {
    param(
        [Parameter(Mandatory = $true)][scriptblock] $Action,
        [Parameter(Mandatory = $true)][string] $Pattern,
        [Parameter(Mandatory = $true)][string] $Message
    )

    try {
        & $Action
    }
    catch {
        if ($_.Exception.Message -notmatch [regex]::Escape($Pattern)) {
            throw "$Message expected error containing=[$Pattern] actual=[$($_.Exception.Message)]"
        }
        return
    }

    throw "$Message expected an exception containing=[$Pattern]"
}

function New-MatrixFixture {
    param(
        [Parameter(Mandatory = $true)][string] $Case
    )

    $imageCheckpoint = [ordered]@{
        id = "final"
        kind = "image"
        placeholder = '$CAPTURE_FILE$'
        artifact = "01.png"
        required = $true
    }
    $scenario = [ordered]@{
        id = "01"
        script = "scenarios/01.cel"
        capabilities = @("CAP-SELF")
        checkpoints = @($imageCheckpoint)
        knownGaps = @("fixture")
    }
    $runtime = [ordered]@{
        id = "runtime"
        config = "runtime.yaml"
        required = $true
        checkpoints = @(
            [ordered]@{
                id = "trace-created"
                kind = "fileExists"
                path = '${TRACE_FILE}'
                required = $true
            }
        )
        imageCheckpoints = @()
        knownGaps = @("fixture")
    }
    $matrix = [ordered]@{
        schemaVersion = 1
        unifiedExeScenarios = @($scenario)
        runtimeScenarios = @($runtime)
    }

    switch ($Case) {
        "duplicate-scenario" {
            $matrix.unifiedExeScenarios = @($scenario, [ordered]@{
                id = "01"
                script = "scenarios/02.cel"
                capabilities = @("CAP-SELF")
                checkpoints = @([ordered]@{
                    id = "final"
                    kind = "image"
                    placeholder = '$CAPTURE_FILE$'
                    artifact = "02.png"
                    required = $true
                })
                knownGaps = @("fixture")
            })
        }
        "duplicate-checkpoint" {
            $scenario.checkpoints = @($imageCheckpoint, [ordered]@{
                id = "final"
                kind = "image"
                placeholder = '$CAPTURE_FILE$'
                artifact = "01-second.png"
                required = $true
            })
        }
        "missing-script" { $scenario.script = "scenarios/missing.cel" }
        "unsupported-kind" { $imageCheckpoint.kind = "unknown" }
        "duplicate-artifact" {
            $matrix.unifiedExeScenarios = @($scenario, [ordered]@{
                id = "02"
                script = "scenarios/02.cel"
                capabilities = @("CAP-SELF")
                checkpoints = @([ordered]@{
                    id = "final"
                    kind = "image"
                    placeholder = '$CAPTURE_FILE$'
                    artifact = "01.png"
                    required = $true
                })
                knownGaps = @("fixture")
            })
        }
        "duplicate-runtime-image-artifact" {
            $runtime.imageCheckpoints = @(
                [ordered]@{ id = "first"; kind = "image"; artifact = "runtime.png"; required = $true },
                [ordered]@{ id = "second"; kind = "image"; artifact = "runtime.png"; required = $true }
            )
        }
        "zero-checkpoint" { $scenario.checkpoints = @() }
        "path-escape" { $scenario.script = "..\..\outside.cel" }
    }

    return $matrix
}

Assert-Exists (Join-Path $regressionRoot "run_celestia_compat_regression.ps1") "Missing regression runner"
Assert-Exists (Join-Path $scriptRoot "image_metrics.py") "Missing image metrics helper"
Assert-Exists (Join-Path $regressionRoot "verification-matrix.json") "Missing verification matrix"
Assert-Exists (Join-Path $scriptRoot "Run-CelestiaCompatFaultInjection.ps1") "Missing fault injection runner"
$python = Get-Command python -ErrorAction SilentlyContinue
if ($null -eq $python) {
    throw "python command was not found"
}

$pass = New-RegressionCheck -Id "pass" -Group "self" -Status pass -Required $true -Detail "ok"
$warn = New-RegressionCheck -Id "warn" -Group "self" -Status warn -Required $true -Detail "review"
$fail = New-RegressionCheck -Id "fail" -Group "self" -Status fail -Required $true -Detail "broken"
$errorCheck = New-RegressionCheck -Id "error" -Group "self" -Status error -Required $true -Detail "harness"
$skippedRequired = New-RegressionCheck -Id "skip" -Group "self" -Status skipped -Required $true -Detail "skipped"

Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @()) -Expected "error" -Message "empty aggregate"
Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @($pass)) -Expected "pass" -Message "pass aggregate"
Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @($pass, $warn)) -Expected "warn" -Message "warn aggregate"
Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @($pass, $fail)) -Expected "fail" -Message "fail aggregate"
Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @($fail, $errorCheck)) -Expected "error" -Message "error aggregate"
Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks @($pass, $skippedRequired)) -Expected "warn" -Message "required skipped aggregate"
Assert-Equal -Actual (Get-RegressionExitCode -Status pass) -Expected 0 -Message "pass exit"
Assert-Equal -Actual (Get-RegressionExitCode -Status fail) -Expected 1 -Message "fail exit"
Assert-Equal -Actual (Get-RegressionExitCode -Status warn) -Expected 2 -Message "warn exit"
Assert-Equal -Actual (Get-RegressionExitCode -Status error) -Expected 3 -Message "error exit"

$matrixTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("celestia-matrix-selftest-" + [guid]::NewGuid().ToString("N"))
try {
    $matrixScenarioRoot = Join-Path $matrixTestRoot "scenarios"
    New-Item -ItemType Directory -Path $matrixScenarioRoot -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $matrixScenarioRoot "01.cel") -Value 'print { text "fixture" }' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $matrixScenarioRoot "02.cel") -Value 'print { text "fixture" }' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $matrixTestRoot "runtime.yaml") -Value 'fixture: true' -Encoding UTF8

    function Invoke-MatrixFixtureValidation {
        param(
            [Parameter(Mandatory = $true)][string] $Case,
            [switch] $RemoveSecondScript,
            [switch] $AddUnexpectedScript
        )

        $caseRoot = Join-Path $matrixTestRoot $Case
        New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
        Copy-Item -LiteralPath $matrixScenarioRoot -Destination (Join-Path $caseRoot "scenarios") -Recurse
        Copy-Item -LiteralPath (Join-Path $matrixTestRoot "runtime.yaml") -Destination (Join-Path $caseRoot "runtime.yaml")
        if ($RemoveSecondScript) {
            Remove-Item -LiteralPath (Join-Path $caseRoot "scenarios\02.cel") -Force
        }
        if ($AddUnexpectedScript) {
            Set-Content -LiteralPath (Join-Path $caseRoot "scenarios\unexpected.cel") -Value 'print { text "unexpected" }' -Encoding UTF8
        }

        $matrixPath = Join-Path $caseRoot "verification-matrix.json"
        New-MatrixFixture -Case $Case | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $matrixPath -Encoding UTF8
        $matrix = Read-VerificationMatrix -Path $matrixPath
        Test-VerificationMatrix -Matrix $matrix -MatrixPath $matrixPath -RepoRoot $caseRoot -ScenarioRoot (Join-Path $caseRoot "scenarios")
    }

    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "duplicate-scenario" } -Pattern "duplicate scenario id" -Message "duplicate scenario validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "duplicate-checkpoint" } -Pattern "duplicate checkpoint id" -Message "duplicate checkpoint validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "missing-script" -RemoveSecondScript } -Pattern "missing scenario script" -Message "missing scenario validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "base" -RemoveSecondScript -AddUnexpectedScript } -Pattern "unexpected scenario script" -Message "unexpected scenario validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "unsupported-kind" -RemoveSecondScript } -Pattern "unsupported checkpoint kind" -Message "checkpoint kind validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "duplicate-artifact" } -Pattern "duplicate checkpoint artifact" -Message "checkpoint artifact validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "duplicate-runtime-image-artifact" -RemoveSecondScript } -Pattern "duplicate checkpoint artifact" -Message "runtime checkpoint artifact validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "zero-checkpoint" -RemoveSecondScript } -Pattern "scenario has no checkpoints" -Message "zero checkpoint validation"
    Assert-ThrowsLike -Action { Invoke-MatrixFixtureValidation -Case "path-escape" -RemoveSecondScript } -Pattern "path escapes allowed root" -Message "path escape validation"

    $validRoot = Join-Path $matrixTestRoot "valid"
    New-Item -ItemType Directory -Path (Join-Path $validRoot "scenarios") -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $matrixScenarioRoot "01.cel") -Destination (Join-Path $validRoot "scenarios\01.cel")
    Copy-Item -LiteralPath (Join-Path $matrixTestRoot "runtime.yaml") -Destination (Join-Path $validRoot "runtime.yaml")
    $validMatrixPath = Join-Path $validRoot "verification-matrix.json"
    New-MatrixFixture -Case "base" | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $validMatrixPath -Encoding UTF8
    $validMatrix = Read-VerificationMatrix -Path $validMatrixPath
    Assert-Equal -Actual (Test-VerificationMatrix -Matrix $validMatrix -MatrixPath $validMatrixPath -RepoRoot $validRoot -ScenarioRoot (Join-Path $validRoot "scenarios")) -Expected $true -Message "valid matrix"
}
finally {
    if (Test-Path -LiteralPath $matrixTestRoot) {
        Remove-Item -LiteralPath $matrixTestRoot -Recurse -Force
    }
}

$gateProbe = Join-Path $scriptRoot "Invoke-CelestiaCompatGateProbe.ps1"
Assert-Exists $gateProbe "Missing gate probe: $gateProbe"
$gateTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("celestia-gate-selftest-" + [guid]::NewGuid().ToString("N"))
try {
    $gateCases = @(
        @{ Status = "pass"; ExpectedExitCode = 0 },
        @{ Status = "fail"; ExpectedExitCode = 1 },
        @{ Status = "warn"; ExpectedExitCode = 2 },
        @{ Status = "error"; ExpectedExitCode = 3 }
    )
    foreach ($gateCase in $gateCases) {
        $outputRoot = Join-Path $gateTestRoot $gateCase.Status
        $stdoutPath = Join-Path $gateTestRoot ($gateCase.Status + ".stdout.log")
        $stderrPath = Join-Path $gateTestRoot ($gateCase.Status + ".stderr.log")
        New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
        $process = Start-Process `
            -FilePath "powershell" `
            -ArgumentList @(
                "-NoProfile",
                "-ExecutionPolicy", "Bypass",
                "-File", $gateProbe,
                "-Status", $gateCase.Status,
                "-OutputRoot", $outputRoot
            ) `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath `
            -Wait `
            -PassThru
        Assert-Equal -Actual $process.ExitCode -Expected $gateCase.ExpectedExitCode -Message ("gate probe exit " + $gateCase.Status)

        $jsonPath = Join-Path $outputRoot "machine-report.json"
        $markdownPath = Join-Path $outputRoot "machine-report.md"
        Assert-Exists $jsonPath ("Missing gate JSON report: " + $gateCase.Status)
        Assert-Exists $markdownPath ("Missing gate Markdown report: " + $gateCase.Status)
        $summary = Get-Content -LiteralPath $jsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
        $markdown = Get-Content -LiteralPath $markdownPath -Raw -Encoding UTF8
        Assert-Equal -Actual $summary.status -Expected $gateCase.Status -Message ("gate JSON status " + $gateCase.Status)
        Assert-Equal -Actual $summary.exitCode -Expected $gateCase.ExpectedExitCode -Message ("gate JSON exit " + $gateCase.Status)
        Assert-Equal -Actual $summary.runId -Expected ("probe-" + $gateCase.Status) -Message ("gate JSON runId " + $gateCase.Status)
        Assert-Equal -Actual $summary.mode -Expected "Probe" -Message ("gate JSON mode " + $gateCase.Status)
        Assert-Equal -Actual @($summary.checks).Count -Expected 1 -Message ("gate JSON check count " + $gateCase.Status)
        foreach ($expectedLine in @(
            "| Run ID | $($summary.runId) |",
            "| Mode | $($summary.mode) |",
            "| Status | $($summary.status) |",
            "| Exit code | $($summary.exitCode) |",
            "| Check count | $(@($summary.checks).Count) |"
        )) {
            if ($markdown -notmatch [regex]::Escape($expectedLine)) {
                throw "Gate Markdown header mismatch: status=$($gateCase.Status) missing=[$expectedLine]"
            }
        }
    }
}
finally {
    if (Test-Path -LiteralPath $gateTestRoot) {
        Remove-Item -LiteralPath $gateTestRoot -Recurse -Force
    }
}

$baselineTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("celestia-baseline-selftest-" + [guid]::NewGuid().ToString("N"))
try {
    $baselineImageRoot = Join-Path $baselineTestRoot "baseline"
    $baselineScreenshotRoot = Join-Path $baselineImageRoot "screenshots\baseline"
    New-Item -ItemType Directory -Path $baselineScreenshotRoot -Force | Out-Null
    $baselineEntries = @(
        [pscustomobject]@{ scenarioId = "scene-a"; checkpointId = "final"; relativePath = "screenshots/baseline/a.png" },
        [pscustomobject]@{ scenarioId = "scene-b"; checkpointId = "final"; relativePath = "screenshots/baseline/b.png" },
        [pscustomobject]@{ scenarioId = "scene-c"; checkpointId = "t1"; relativePath = "screenshots/baseline/c-t1.png" }
    )
    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "a.png") -Value "fixture-a" -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "b.png") -Value "fixture-b" -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "c-t1.png") -Value "fixture-c" -Encoding UTF8
    $baselineManifestPath = Join-Path $baselineImageRoot "baseline-manifest.json"
    New-BaselineManifest `
        -BaselineCommit "baseline-commit" `
        -BaselineRoot $baselineImageRoot `
        -ImageEntries $baselineEntries `
        -OutputPath $baselineManifestPath | Out-Null

    $baselineChecks = @(Test-BaselineManifest -ManifestPath $baselineManifestPath -BaselineCommit "baseline-commit" -BaselineRoot $baselineImageRoot -ExpectedImages $baselineEntries)
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $baselineChecks) -Expected "pass" -Message "complete baseline manifest"

    Remove-Item -LiteralPath (Join-Path $baselineScreenshotRoot "b.png") -Force
    $baselineChecks = @(Test-BaselineManifest -ManifestPath $baselineManifestPath -BaselineCommit "baseline-commit" -BaselineRoot $baselineImageRoot -ExpectedImages $baselineEntries)
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $baselineChecks) -Expected "fail" -Message "missing baseline image"
    if ((@($baselineChecks | Where-Object Detail -match "missing baseline image")).Count -eq 0) { throw "missing baseline image detail was not reported" }
    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "b.png") -Value "fixture-b" -Encoding UTF8

    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "old.png") -Value "fixture-old" -Encoding UTF8
    $baselineChecks = @(Test-BaselineManifest -ManifestPath $baselineManifestPath -BaselineCommit "baseline-commit" -BaselineRoot $baselineImageRoot -ExpectedImages $baselineEntries)
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $baselineChecks) -Expected "fail" -Message "unexpected baseline image"
    if ((@($baselineChecks | Where-Object Detail -match "unexpected baseline image")).Count -eq 0) { throw "unexpected baseline image detail was not reported" }
    Remove-Item -LiteralPath (Join-Path $baselineScreenshotRoot "old.png") -Force

    Set-Content -LiteralPath (Join-Path $baselineScreenshotRoot "a.png") -Value "fixture-a-modified" -Encoding UTF8
    $baselineChecks = @(Test-BaselineManifest -ManifestPath $baselineManifestPath -BaselineCommit "baseline-commit" -BaselineRoot $baselineImageRoot -ExpectedImages $baselineEntries)
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $baselineChecks) -Expected "fail" -Message "baseline hash mismatch"
    if ((@($baselineChecks | Where-Object Detail -match "hash mismatch")).Count -eq 0) { throw "baseline hash mismatch detail was not reported" }

    $baselineChecks = @(Test-BaselineManifest -ManifestPath $baselineManifestPath -BaselineCommit "different-commit" -BaselineRoot $baselineImageRoot -ExpectedImages $baselineEntries)
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $baselineChecks) -Expected "fail" -Message "baseline commit mismatch"
    if ((@($baselineChecks | Where-Object Detail -match "baseline commit mismatch")).Count -eq 0) { throw "baseline commit mismatch detail was not reported" }
}
finally {
    if (Test-Path -LiteralPath $baselineTestRoot) {
        Remove-Item -LiteralPath $baselineTestRoot -Recurse -Force
    }
}

$checkpointTestRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("celestia-checkpoint-selftest-" + [guid]::NewGuid().ToString("N"))
try {
    New-Item -ItemType Directory -Path $checkpointTestRoot -Force | Out-Null
    $stdoutFixture = Join-Path $checkpointTestRoot "runtime.stdout.log"
    $stderrFixture = Join-Path $checkpointTestRoot "runtime.stderr.log"
    $traceFixture = Join-Path $checkpointTestRoot "runtime.trace"
    $firstImage = Join-Path $checkpointTestRoot "first.png"
    $secondImage = Join-Path $checkpointTestRoot "second.png"
    $metricsRoot = Join-Path $checkpointTestRoot "metrics"
    $positiveStdout = @'
view.frameRendered count=24
view.frameRendered payload=observerReferenceBodyId=Sol selectionType=body selectionId=Earth bodyCount=1 starCount=1 resourceCount=3 missingRequiredResourceCount=0
all hosts stopped
'@
    Set-Content -LiteralPath $stdoutFixture -Value $positiveStdout -Encoding UTF8
    Set-Content -LiteralPath $stderrFixture -Value "" -Encoding UTF8
    Set-Content -LiteralPath $traceFixture -Value "trace fixture" -Encoding UTF8
    $fixtureCode = "from PIL import Image; import sys; Image.new('RGB',(160,120),(40,80,160)).save(sys.argv[1]); Image.new('RGB',(160,120),(160,80,40)).save(sys.argv[2])"
    & $python.Source -c $fixtureCode $firstImage $secondImage
    if ($LASTEXITCODE -ne 0) { throw "Pillow fixture creation failed" }

    $checkpointArtifacts = @{
        ScenarioId = "runtime-fixture"
        Stdout = $stdoutFixture
        Stderr = $stderrFixture
        Trace = $traceFixture
        ArtifactRoot = $checkpointTestRoot
        ImageRoot = $checkpointTestRoot
        MetricsRoot = $metricsRoot
        ImageMetricsScript = (Join-Path $scriptRoot "image_metrics.py")
        Python = $python.Source
    }
    $runtimeCheckpoints = @(
        [pscustomobject]@{ id = "clean-shutdown"; kind = "stdoutRegex"; pattern = "all hosts stopped"; minimumMatches = 1; required = $true },
        [pscustomobject]@{ id = "positive-frame-count"; kind = "stdoutRegex"; pattern = "view\.frameRendered count=[1-9][0-9]*"; minimumMatches = 1; required = $true },
        [pscustomobject]@{ id = "synthetic-earth-identity"; kind = "stdoutRegex"; pattern = "view\.frameRendered payload=.*observerReferenceBodyId=Sol.*selectionType=body.*selectionId=Earth"; minimumMatches = 1; required = $true },
        [pscustomobject]@{ id = "trace-created"; kind = "fileExists"; path = '${TRACE_FILE}'; required = $true },
        [pscustomobject]@{ id = "first-image"; kind = "image"; artifact = "first.png"; required = $true },
        [pscustomobject]@{ id = "second-image"; kind = "image"; artifact = "second.png"; required = $true }
    )

    $runtimeFixtureChecks = @($runtimeCheckpoints | ForEach-Object { Test-RegressionCheckpoint -Checkpoint $_ -Artifacts $checkpointArtifacts })
    Assert-Equal -Actual (Get-RegressionAggregateStatus -Checks $runtimeFixtureChecks) -Expected "pass" -Message "runtime checkpoint fixture"
    Write-Output "runtime checkpoint fixture: pass"
    Write-Output "runtime two-image fixture: pass"

    Set-Content -LiteralPath $stdoutFixture -Value ($positiveStdout -replace "all hosts stopped", "shutdown token removed") -Encoding UTF8
    Assert-Equal -Actual (Test-RegressionCheckpoint -Checkpoint $runtimeCheckpoints[0] -Artifacts $checkpointArtifacts).Status -Expected "fail" -Message "missing clean shutdown token"
    Set-Content -LiteralPath $stdoutFixture -Value ($positiveStdout -replace "count=24", "count=0") -Encoding UTF8
    Assert-Equal -Actual (Test-RegressionCheckpoint -Checkpoint $runtimeCheckpoints[1] -Artifacts $checkpointArtifacts).Status -Expected "fail" -Message "zero frame count"
    Set-Content -LiteralPath $stdoutFixture -Value $positiveStdout -Encoding UTF8

    Remove-Item -LiteralPath $traceFixture -Force
    Assert-Equal -Actual (Test-RegressionCheckpoint -Checkpoint $runtimeCheckpoints[3] -Artifacts $checkpointArtifacts).Status -Expected "fail" -Message "missing trace"
    Set-Content -LiteralPath $traceFixture -Value "trace fixture" -Encoding UTF8

    Remove-Item -LiteralPath $firstImage -Force
    Assert-Equal -Actual (Test-RegressionCheckpoint -Checkpoint $runtimeCheckpoints[4] -Artifacts $checkpointArtifacts).Status -Expected "fail" -Message "missing first image"
    & $python.Source -c "from PIL import Image; import sys; Image.new('RGB',(160,120),(40,80,160)).save(sys.argv[1])" $firstImage
    if ($LASTEXITCODE -ne 0) { throw "Pillow first-image restoration failed" }
    Remove-Item -LiteralPath $secondImage -Force
    Assert-Equal -Actual (Test-RegressionCheckpoint -Checkpoint $runtimeCheckpoints[5] -Artifacts $checkpointArtifacts).Status -Expected "fail" -Message "missing second image"
}
finally {
    if (Test-Path -LiteralPath $checkpointTestRoot) {
        Remove-Item -LiteralPath $checkpointTestRoot -Recurse -Force
    }
}

$runnerText = Get-Content -LiteralPath (Join-Path $regressionRoot "run_celestia_compat_regression.ps1") -Raw
if ($runnerText -notmatch 'ValidateSet\("SelfTest", "InitBaseline", "Quick", "Full", "Step18"\)') {
    throw "Regression runner does not expose Step18 mode"
}
foreach ($required in @("Invoke-Step18", "Invoke-SelectedMode", "New-RegressionRunSummary", "Write-RegressionReports")) {
    if ($runnerText -notmatch $required) {
        throw "Regression runner is missing formal gate helper: $required"
    }
}
foreach ($required in @("ReadToEndAsync")) {
    if ($runnerText -notmatch $required) {
        throw "Regression runner does not drain redirected process output asynchronously: $required"
    }
}
$reportImplementationText = $runnerText + (Get-Content -LiteralPath (Join-Path $scriptRoot "CelestiaCompatRegression.psm1") -Raw)
foreach ($required in @("Step18 Claim Boundary", "does not prove complete historical renderer parity")) {
    if ($reportImplementationText -notmatch [regex]::Escape($required)) {
        throw "Step18 report boundary text is missing: $required"
    }
}

$scenarioRoot = Join-Path $regressionRoot "scenarios"
Assert-Exists $scenarioRoot "Missing scenario directory"

$verificationMatrixPath = Join-Path $regressionRoot "verification-matrix.json"
$verificationMatrix = Read-VerificationMatrix -Path $verificationMatrixPath
Test-VerificationMatrix `
    -Matrix $verificationMatrix `
    -MatrixPath $verificationMatrixPath `
    -RepoRoot $repoRoot `
    -ScenarioRoot $scenarioRoot | Out-Null
Write-Output ("verification matrix: {0} unified exe scenarios, {1} runtime scenarios" -f @($verificationMatrix.unifiedExeScenarios).Count, @($verificationMatrix.runtimeScenarios).Count)

$scenarios = @(Get-ChildItem -LiteralPath $scenarioRoot -Filter "*.cel" -File | Sort-Object Name)
foreach ($scenario in $scenarios) {
    $text = Get-Content -LiteralPath $scenario.FullName -Raw
    if ($text -notmatch '\$CAPTURE_FILE\$') {
        throw "Scenario $($scenario.Name) does not contain capture placeholder"
    }
    if ($text -notmatch 'exit\s*\{\s*\}') {
        throw "Scenario $($scenario.Name) does not exit after capture"
    }
}

& $python.Source (Join-Path $scriptRoot "image_metrics.py") --self-test
if ($LASTEXITCODE -ne 0) {
    throw "image metrics helper self-test failed"
}

Write-Output "Celestia compatibility regression self-test passed"
