Set-StrictMode -Version Latest

function New-RegressionCheck {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $Id,
        [Parameter(Mandatory = $true)][string] $Group,
        [Parameter(Mandatory = $true)]
        [ValidateSet("pass", "warn", "fail", "error", "skipped")]
        [string] $Status,
        [Parameter(Mandatory = $true)][bool] $Required,
        [Parameter(Mandatory = $true)][string] $Detail,
        [string[]] $Evidence = @()
    )

    [pscustomobject][ordered]@{
        Id = $Id
        Group = $Group
        Status = $Status
        Required = $Required
        Detail = $Detail
        Evidence = @($Evidence)
    }
}

function Get-RegressionAggregateStatus {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [object[]] $Checks
    )

    if ($Checks.Count -eq 0) { return "error" }
    if (@($Checks | Where-Object Status -eq "error").Count -gt 0) { return "error" }
    if (@($Checks | Where-Object Status -eq "fail").Count -gt 0) { return "fail" }
    if (@($Checks | Where-Object Status -eq "warn").Count -gt 0) { return "warn" }
    if (@($Checks | Where-Object { $_.Required -and $_.Status -eq "skipped" }).Count -gt 0) { return "warn" }
    return "pass"
}

function Get-RegressionExitCode {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("pass", "warn", "fail", "error")]
        [string] $Status
    )

    switch ($Status) {
        "pass" { return 0 }
        "fail" { return 1 }
        "warn" { return 2 }
        "error" { return 3 }
    }
}

function Invoke-RegressionStage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $Id,
        [Parameter(Mandatory = $true)][string] $Group,
        [Parameter(Mandatory = $true)][scriptblock] $Action,
        [ValidateSet("fail", "error")][string] $FailureStatus = "fail",
        [bool] $Required = $true,
        [string] $SuccessDetail = "completed"
    )

    try {
        $value = & $Action
        return [pscustomobject][ordered]@{
            Value = $value
            Check = New-RegressionCheck -Id $Id -Group $Group -Status pass -Required $Required -Detail $SuccessDetail
        }
    }
    catch {
        return [pscustomobject][ordered]@{
            Value = $null
            Check = New-RegressionCheck -Id $Id -Group $Group -Status $FailureStatus -Required $Required -Detail $_.Exception.Message
        }
    }
}

function New-RegressionRunSummary {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $RunId,
        [Parameter(Mandatory = $true)][string] $Mode,
        [Parameter(Mandatory = $true)][string] $CurrentCommit,
        [Parameter(Mandatory = $true)][string] $BaselineCommit,
        [Parameter(Mandatory = $true)][string] $VerificationMatrix,
        [Parameter(Mandatory = $true)][string] $ArtifactRoot,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $Checks
    )

    $status = Get-RegressionAggregateStatus -Checks $Checks
    [pscustomobject][ordered]@{
        schemaVersion = 1
        runId = $RunId
        mode = $Mode
        currentCommit = $CurrentCommit
        baselineCommit = $BaselineCommit
        status = $status
        exitCode = Get-RegressionExitCode -Status $status
        verificationMatrix = $VerificationMatrix
        artifactRoot = $ArtifactRoot
        checks = @($Checks)
    }
}

function ConvertTo-RegressionMarkdownCell {
    param([AllowNull()] $Value)
    if ($null -eq $Value) { return "" }
    return ([string]$Value).Replace("|", "\|").Replace("`r", " ").Replace("`n", " ")
}

function Write-RegressionReports {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][object] $Summary,
        [Parameter(Mandatory = $true)][string] $OutputRoot
    )

    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
    $jsonPath = [System.IO.Path]::GetFullPath((Join-Path $OutputRoot "machine-report.json"))
    $markdownPath = [System.IO.Path]::GetFullPath((Join-Path $OutputRoot "machine-report.md"))
    $Summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $jsonPath -Encoding UTF8

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# Celestia Compatibility Regression Report")
    $lines.Add("")
    $lines.Add("| Field | Value |")
    $lines.Add("|---|---|")
    $lines.Add("| Run ID | $(ConvertTo-RegressionMarkdownCell $Summary.runId) |")
    $lines.Add("| Mode | $(ConvertTo-RegressionMarkdownCell $Summary.mode) |")
    $lines.Add("| Status | $(ConvertTo-RegressionMarkdownCell $Summary.status) |")
    $lines.Add("| Exit code | $($Summary.exitCode) |")
    $lines.Add("| Check count | $(@($Summary.checks).Count) |")
    $lines.Add("| Current commit | $(ConvertTo-RegressionMarkdownCell $Summary.currentCommit) |")
    $lines.Add("| Baseline commit | $(ConvertTo-RegressionMarkdownCell $Summary.baselineCommit) |")
    $lines.Add("| Verification matrix | $(ConvertTo-RegressionMarkdownCell $Summary.verificationMatrix) |")
    $lines.Add("| Artifact root | $(ConvertTo-RegressionMarkdownCell $Summary.artifactRoot) |")
    $lines.Add("")
    $lines.Add("## Checks")
    $lines.Add("")
    $lines.Add("| ID | Group | Required | Status | Detail | Evidence |")
    $lines.Add("|---|---|---:|---|---|---|")
    foreach ($check in @($Summary.checks)) {
        $evidence = @($check.Evidence) -join "; "
        $lines.Add("| $(ConvertTo-RegressionMarkdownCell $check.Id) | $(ConvertTo-RegressionMarkdownCell $check.Group) | $($check.Required) | $(ConvertTo-RegressionMarkdownCell $check.Status) | $(ConvertTo-RegressionMarkdownCell $check.Detail) | $(ConvertTo-RegressionMarkdownCell $evidence) |")
    }
    $lines.Add("")
    if ($Summary.mode -eq "Step18") {
        $lines.Add("## Step18 Claim Boundary")
        $lines.Add("")
        $lines.Add("A passing Step18 run means no covered check failed. It does not prove complete historical renderer parity, exhaustive Celestia feature parity, pixel-perfect rendering, or Qt/Win32 frontend parity.")
    }
    else {
        $lines.Add("## Claim Boundary")
        $lines.Add("")
        $lines.Add("A passing run means no covered check failed. It does not prove exhaustive Celestia feature parity, pixel-perfect rendering, or Qt/Win32 frontend parity.")
    }
    $lines | Set-Content -LiteralPath $markdownPath -Encoding UTF8

    return [pscustomobject][ordered]@{
        Json = $jsonPath
        Markdown = $markdownPath
    }
}

function ConvertTo-NormalizedRegressionPath {
    param([Parameter(Mandatory = $true)][string] $Path)
    return $Path.Replace("\", "/").TrimStart("./").ToLowerInvariant()
}

function Get-RelativeRegressionPath {
    param(
        [Parameter(Mandatory = $true)][string] $BasePath,
        [Parameter(Mandatory = $true)][string] $Path
    )

    $baseFull = [System.IO.Path]::GetFullPath($BasePath).TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $baseUri = New-Object System.Uri($baseFull)
    $pathUri = New-Object System.Uri($pathFull)
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($pathUri).ToString())
}

function Resolve-RegressionPathWithinRoot {
    param(
        [Parameter(Mandatory = $true)][string] $BasePath,
        [Parameter(Mandatory = $true)][string] $RelativePath,
        [Parameter(Mandatory = $true)][string] $AllowedRoot,
        [Parameter(Mandatory = $true)][string] $Label
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        throw "$Label path is empty"
    }

    $allowedFull = [System.IO.Path]::GetFullPath($AllowedRoot).TrimEnd("\", "/")
    $candidate = if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        [System.IO.Path]::GetFullPath($RelativePath)
    }
    else {
        [System.IO.Path]::GetFullPath((Join-Path $BasePath $RelativePath))
    }
    $allowedPrefix = $allowedFull + [System.IO.Path]::DirectorySeparatorChar
    if (-not $candidate.Equals($allowedFull, [System.StringComparison]::OrdinalIgnoreCase) -and
        -not $candidate.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "path escapes allowed root: $Label path=$RelativePath allowedRoot=$allowedFull"
    }

    return $candidate
}

function Read-VerificationMatrix {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string] $Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "verification matrix is missing: $Path"
    }

    try {
        $matrix = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
    }
    catch {
        throw "verification matrix JSON is invalid: $Path; $($_.Exception.Message)"
    }

    if ($matrix.schemaVersion -ne 1) {
        throw "unsupported verification matrix schemaVersion: $($matrix.schemaVersion)"
    }
    return $matrix
}

function Test-ArtifactSet {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]] $Expected,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]] $Actual,
        [string] $MissingLabel = "missing artifact",
        [string] $UnexpectedLabel = "unexpected artifact"
    )

    $expectedNormalized = @($Expected | ForEach-Object { ConvertTo-NormalizedRegressionPath $_ } | Sort-Object -Unique)
    $actualNormalized = @($Actual | ForEach-Object { ConvertTo-NormalizedRegressionPath $_ } | Sort-Object -Unique)
    $missing = @($expectedNormalized | Where-Object { $_ -notin $actualNormalized })
    $unexpected = @($actualNormalized | Where-Object { $_ -notin $expectedNormalized })

    if ($missing.Count -gt 0) {
        throw ("{0}: {1}" -f $MissingLabel, ($missing -join ", "))
    }
    if ($unexpected.Count -gt 0) {
        throw ("{0}: {1}" -f $UnexpectedLabel, ($unexpected -join ", "))
    }
    return $true
}

function Assert-RegressionCheckpointDefinition {
    param(
        [Parameter(Mandatory = $true)][object] $Checkpoint,
        [Parameter(Mandatory = $true)][string] $ScenarioId,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.List[string]] $Artifacts
    )

    $supportedKinds = @("image", "stdoutRegex", "stderrRegex", "traceRegex", "fileExists")
    if ([string]::IsNullOrWhiteSpace([string]$Checkpoint.id)) {
        throw "checkpoint id is empty: scenario=$ScenarioId"
    }
    if ($Checkpoint.kind -notin $supportedKinds) {
        throw "unsupported checkpoint kind: scenario=$ScenarioId checkpoint=$($Checkpoint.id) kind=$($Checkpoint.kind)"
    }
    if ($null -eq $Checkpoint.PSObject.Properties["required"]) {
        throw "checkpoint required flag is missing: scenario=$ScenarioId checkpoint=$($Checkpoint.id)"
    }

    switch ($Checkpoint.kind) {
        "image" {
            if ([string]::IsNullOrWhiteSpace([string]$Checkpoint.artifact)) {
                throw "image checkpoint artifact is missing: scenario=$ScenarioId checkpoint=$($Checkpoint.id)"
            }
            if ([System.IO.Path]::IsPathRooted([string]$Checkpoint.artifact) -or
                (ConvertTo-NormalizedRegressionPath ([string]$Checkpoint.artifact)).StartsWith("../")) {
                throw "path escapes allowed root: checkpoint artifact=$($Checkpoint.artifact)"
            }
            $Artifacts.Add((ConvertTo-NormalizedRegressionPath ([string]$Checkpoint.artifact)))
        }
        { $_ -in @("stdoutRegex", "stderrRegex", "traceRegex") } {
            if ([string]::IsNullOrWhiteSpace([string]$Checkpoint.pattern)) {
                throw "regex checkpoint pattern is missing: scenario=$ScenarioId checkpoint=$($Checkpoint.id)"
            }
            if ($null -eq $Checkpoint.PSObject.Properties["minimumMatches"] -or [int]$Checkpoint.minimumMatches -lt 1) {
                throw "regex checkpoint minimumMatches is invalid: scenario=$ScenarioId checkpoint=$($Checkpoint.id)"
            }
        }
        "fileExists" {
            if ([string]::IsNullOrWhiteSpace([string]$Checkpoint.path)) {
                throw "file checkpoint path is missing: scenario=$ScenarioId checkpoint=$($Checkpoint.id)"
            }
        }
    }
}

function Test-VerificationMatrix {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][object] $Matrix,
        [Parameter(Mandatory = $true)][string] $MatrixPath,
        [Parameter(Mandatory = $true)][string] $RepoRoot,
        [Parameter(Mandatory = $true)][string] $ScenarioRoot
    )

    if ($Matrix.schemaVersion -ne 1) {
        throw "unsupported verification matrix schemaVersion: $($Matrix.schemaVersion)"
    }

    $matrixBase = Split-Path -Parent ([System.IO.Path]::GetFullPath($MatrixPath))
    $unified = @($Matrix.unifiedExeScenarios)
    $runtime = @($Matrix.runtimeScenarios)
    $allScenarios = @($unified) + @($runtime)
    $ids = @($allScenarios | ForEach-Object { [string]$_.id })
    $duplicateIds = @($ids | Group-Object | Where-Object Count -gt 1 | ForEach-Object Name)
    if ($duplicateIds.Count -gt 0) {
        throw "duplicate scenario id: $($duplicateIds -join ', ')"
    }

    $artifacts = New-Object System.Collections.Generic.List[string]
    foreach ($scenario in $allScenarios) {
        if ([string]::IsNullOrWhiteSpace([string]$scenario.id)) {
            throw "scenario id is empty"
        }
        $checkpoints = @($scenario.checkpoints)
        if ($null -ne $scenario.PSObject.Properties["imageCheckpoints"]) {
            $checkpoints += @($scenario.imageCheckpoints)
        }
        if ($checkpoints.Count -eq 0) {
            throw "scenario has no checkpoints: $($scenario.id)"
        }

        $checkpointIds = @($checkpoints | ForEach-Object { [string]$_.id })
        $duplicateCheckpointIds = @($checkpointIds | Group-Object | Where-Object Count -gt 1 | ForEach-Object Name)
        if ($duplicateCheckpointIds.Count -gt 0) {
            throw "duplicate checkpoint id: scenario=$($scenario.id) ids=$($duplicateCheckpointIds -join ', ')"
        }
        foreach ($checkpoint in $checkpoints) {
            Assert-RegressionCheckpointDefinition -Checkpoint $checkpoint -ScenarioId $scenario.id -Artifacts $artifacts
        }
    }

    foreach ($scenario in $unified) {
        foreach ($checkpoint in @($scenario.checkpoints | Where-Object kind -eq "image")) {
            if ($null -eq $checkpoint.PSObject.Properties["placeholder"] -or [string]::IsNullOrWhiteSpace([string]$checkpoint.placeholder)) {
                throw "image checkpoint placeholder is missing: scenario=$($scenario.id) checkpoint=$($checkpoint.id)"
            }
        }
    }

    $duplicateArtifacts = @($artifacts | Group-Object | Where-Object Count -gt 1 | ForEach-Object Name)
    if ($duplicateArtifacts.Count -gt 0) {
        throw "duplicate checkpoint artifact: $($duplicateArtifacts -join ', ')"
    }

    $expectedScripts = New-Object System.Collections.Generic.List[string]
    foreach ($scenario in $unified) {
        $scriptPath = Resolve-RegressionPathWithinRoot -BasePath $matrixBase -RelativePath ([string]$scenario.script) -AllowedRoot $RepoRoot -Label "scenario"
        $expectedScripts.Add((Get-RelativeRegressionPath -BasePath $matrixBase -Path $scriptPath))
    }

    if (-not (Test-Path -LiteralPath $ScenarioRoot -PathType Container)) {
        throw "scenario directory is missing: $ScenarioRoot"
    }
    $actualScripts = @(Get-ChildItem -LiteralPath $ScenarioRoot -Filter "*.cel" -File | ForEach-Object {
        Get-RelativeRegressionPath -BasePath $matrixBase -Path $_.FullName
    })
    Test-ArtifactSet -Expected @($expectedScripts) -Actual $actualScripts -MissingLabel "missing scenario script" -UnexpectedLabel "unexpected scenario script" | Out-Null

    foreach ($scenario in $runtime) {
        $configPath = Resolve-RegressionPathWithinRoot -BasePath $matrixBase -RelativePath ([string]$scenario.config) -AllowedRoot $RepoRoot -Label "runtime config"
        if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) {
            throw "missing runtime config: scenario=$($scenario.id) path=$($scenario.config)"
        }
    }

    return $true
}

function New-BaselineManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $BaselineCommit,
        [Parameter(Mandatory = $true)][string] $BaselineRoot,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $ImageEntries,
        [Parameter(Mandatory = $true)][string] $OutputPath
    )

    $images = New-Object System.Collections.Generic.List[object]
    $keys = New-Object System.Collections.Generic.List[string]
    $paths = New-Object System.Collections.Generic.List[string]
    foreach ($entry in $ImageEntries) {
        $key = "{0}/{1}" -f $entry.scenarioId, $entry.checkpointId
        $relativePath = ([string]$entry.relativePath).Replace("\", "/")
        if ([string]::IsNullOrWhiteSpace([string]$entry.scenarioId) -or [string]::IsNullOrWhiteSpace([string]$entry.checkpointId)) {
            throw "baseline image entry has an empty scenario or checkpoint id"
        }
        if ($key -in $keys) {
            throw "duplicate baseline checkpoint key: $key"
        }
        if ((ConvertTo-NormalizedRegressionPath $relativePath) -in $paths) {
            throw "duplicate baseline image path: $relativePath"
        }
        $imagePath = Resolve-RegressionPathWithinRoot -BasePath $BaselineRoot -RelativePath $relativePath -AllowedRoot $BaselineRoot -Label "baseline image"
        if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
            throw "missing baseline image: key=$key path=$relativePath"
        }
        $hash = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash.ToLowerInvariant()
        $images.Add([pscustomobject][ordered]@{
            scenarioId = [string]$entry.scenarioId
            checkpointId = [string]$entry.checkpointId
            relativePath = $relativePath
            sha256 = $hash
        })
        $keys.Add($key)
        $paths.Add((ConvertTo-NormalizedRegressionPath $relativePath))
    }

    $manifest = [pscustomobject][ordered]@{
        schemaVersion = 1
        baselineCommit = $BaselineCommit
        createdAt = (Get-Date).ToString("o")
        images = $images.ToArray()
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $OutputPath) -Force | Out-Null
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
    return $manifest
}

function Test-BaselineManifest {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $ManifestPath,
        [Parameter(Mandatory = $true)][string] $BaselineCommit,
        [Parameter(Mandatory = $true)][string] $BaselineRoot,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $ExpectedImages
    )

    $checks = New-Object System.Collections.Generic.List[object]
    if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
        $checks.Add((New-RegressionCheck -Id "baseline/manifest-readable" -Group "baseline" -Status fail -Required $true -Detail "baseline manifest is missing: $ManifestPath" -Evidence @($ManifestPath)))
        return $checks.ToArray()
    }

    try {
        $manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    }
    catch {
        $checks.Add((New-RegressionCheck -Id "baseline/manifest-readable" -Group "baseline" -Status error -Required $true -Detail ("baseline manifest JSON is invalid: " + $_.Exception.Message) -Evidence @($ManifestPath)))
        return $checks.ToArray()
    }
    if ($manifest.schemaVersion -ne 1) {
        $checks.Add((New-RegressionCheck -Id "baseline/manifest-schema" -Group "baseline" -Status error -Required $true -Detail "unsupported baseline manifest schemaVersion: $($manifest.schemaVersion)" -Evidence @($ManifestPath)))
        return $checks.ToArray()
    }
    $checks.Add((New-RegressionCheck -Id "baseline/manifest-readable" -Group "baseline" -Status pass -Required $true -Detail "baseline manifest parsed" -Evidence @($ManifestPath)))

    if ([string]$manifest.baselineCommit -eq $BaselineCommit) {
        $checks.Add((New-RegressionCheck -Id "baseline/commit" -Group "baseline" -Status pass -Required $true -Detail "baseline commit matches $BaselineCommit" -Evidence @($ManifestPath)))
    }
    else {
        $checks.Add((New-RegressionCheck -Id "baseline/commit" -Group "baseline" -Status fail -Required $true -Detail "baseline commit mismatch: expected=$BaselineCommit actual=$($manifest.baselineCommit)" -Evidence @($ManifestPath)))
    }

    $expectedKeys = @($ExpectedImages | ForEach-Object { "{0}/{1}|{2}" -f $_.scenarioId, $_.checkpointId, (ConvertTo-NormalizedRegressionPath ([string]$_.relativePath)) })
    $manifestImages = @($manifest.images)
    $manifestKeys = @($manifestImages | ForEach-Object { "{0}/{1}|{2}" -f $_.scenarioId, $_.checkpointId, (ConvertTo-NormalizedRegressionPath ([string]$_.relativePath)) })
    $duplicateKeys = @($manifestKeys | Group-Object | Where-Object Count -gt 1 | ForEach-Object Name)
    $missingKeys = @($expectedKeys | Where-Object { $_ -notin $manifestKeys })
    $unexpectedKeys = @($manifestKeys | Where-Object { $_ -notin $expectedKeys })
    if ($duplicateKeys.Count -gt 0 -or $missingKeys.Count -gt 0 -or $unexpectedKeys.Count -gt 0) {
        $detailParts = New-Object System.Collections.Generic.List[string]
        if ($duplicateKeys.Count -gt 0) { $detailParts.Add("duplicate=" + ($duplicateKeys -join ",")) }
        if ($missingKeys.Count -gt 0) { $detailParts.Add("missing=" + ($missingKeys -join ",")) }
        if ($unexpectedKeys.Count -gt 0) { $detailParts.Add("unexpected=" + ($unexpectedKeys -join ",")) }
        $checks.Add((New-RegressionCheck -Id "baseline/manifest-entry-set" -Group "baseline" -Status fail -Required $true -Detail ("baseline manifest entry set mismatch: " + ($detailParts -join "; ")) -Evidence @($ManifestPath)))
    }
    else {
        $checks.Add((New-RegressionCheck -Id "baseline/manifest-entry-set" -Group "baseline" -Status pass -Required $true -Detail ("baseline manifest entry set matches; count=" + $expectedKeys.Count) -Evidence @($ManifestPath)))
    }

    $manifestPaths = @($manifestImages | ForEach-Object { ([string]$_.relativePath).Replace("\", "/") })
    $screenshotRoot = Join-Path $BaselineRoot "screenshots\baseline"
    $diskPaths = @()
    if (Test-Path -LiteralPath $screenshotRoot -PathType Container) {
        $diskPaths = @(Get-ChildItem -LiteralPath $screenshotRoot -Filter "*.png" -File -Recurse | ForEach-Object {
            (Get-RelativeRegressionPath -BasePath $BaselineRoot -Path $_.FullName).Replace("\", "/")
        })
    }
    $manifestPathsNormalized = @($manifestPaths | ForEach-Object { ConvertTo-NormalizedRegressionPath $_ })
    $diskPathsNormalized = @($diskPaths | ForEach-Object { ConvertTo-NormalizedRegressionPath $_ })
    $missingDisk = @($manifestPathsNormalized | Where-Object { $_ -notin $diskPathsNormalized })
    $unexpectedDisk = @($diskPathsNormalized | Where-Object { $_ -notin $manifestPathsNormalized })
    if ($missingDisk.Count -gt 0 -or $unexpectedDisk.Count -gt 0) {
        $detailParts = New-Object System.Collections.Generic.List[string]
        if ($missingDisk.Count -gt 0) { $detailParts.Add("missing baseline image=" + ($missingDisk -join ",")) }
        if ($unexpectedDisk.Count -gt 0) { $detailParts.Add("unexpected baseline image=" + ($unexpectedDisk -join ",")) }
        $checks.Add((New-RegressionCheck -Id "baseline/artifact-set" -Group "baseline" -Status fail -Required $true -Detail ($detailParts -join "; ") -Evidence @($screenshotRoot)))
    }
    else {
        $checks.Add((New-RegressionCheck -Id "baseline/artifact-set" -Group "baseline" -Status pass -Required $true -Detail ("baseline image set matches; count=" + $manifestPaths.Count) -Evidence @($screenshotRoot)))
    }

    foreach ($image in $manifestImages) {
        $key = "{0}/{1}" -f $image.scenarioId, $image.checkpointId
        try {
            $imagePath = Resolve-RegressionPathWithinRoot -BasePath $BaselineRoot -RelativePath ([string]$image.relativePath) -AllowedRoot $BaselineRoot -Label "baseline image"
            if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
                $checks.Add((New-RegressionCheck -Id ("baseline/hash/" + $key) -Group "baseline" -Status fail -Required $true -Detail "missing baseline image for hash: key=$key path=$($image.relativePath)" -Evidence @($imagePath)))
                continue
            }
            $actualHash = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash.ToLowerInvariant()
            $expectedHash = ([string]$image.sha256).ToLowerInvariant()
            if ($actualHash -ne $expectedHash) {
                $checks.Add((New-RegressionCheck -Id ("baseline/hash/" + $key) -Group "baseline" -Status fail -Required $true -Detail "baseline hash mismatch: key=$key expected=$expectedHash actual=$actualHash" -Evidence @($imagePath)))
            }
            else {
                $checks.Add((New-RegressionCheck -Id ("baseline/hash/" + $key) -Group "baseline" -Status pass -Required $true -Detail "baseline hash matches: key=$key" -Evidence @($imagePath)))
            }
        }
        catch {
            $checks.Add((New-RegressionCheck -Id ("baseline/hash/" + $key) -Group "baseline" -Status fail -Required $true -Detail $_.Exception.Message -Evidence @($ManifestPath)))
        }
    }
    return $checks.ToArray()
}

function Get-RegressionCheckpointIdentity {
    param(
        [Parameter(Mandatory = $true)][object] $Checkpoint,
        [Parameter(Mandatory = $true)][hashtable] $Artifacts
    )

    $scenarioId = if ($Artifacts.ContainsKey("ScenarioId") -and -not [string]::IsNullOrWhiteSpace([string]$Artifacts.ScenarioId)) {
        [string]$Artifacts.ScenarioId
    }
    else {
        "scenario"
    }
    return "{0}/{1}" -f $scenarioId, $Checkpoint.id
}

function Get-RegressionCheckpointRequired {
    param([Parameter(Mandatory = $true)][object] $Checkpoint)
    if ($null -eq $Checkpoint.PSObject.Properties["required"]) { return $true }
    return [bool]$Checkpoint.required
}

function Resolve-RegressionCheckpointPath {
    param(
        [Parameter(Mandatory = $true)][string] $Path,
        [Parameter(Mandatory = $true)][hashtable] $Artifacts
    )

    switch ($Path) {
        '${TRACE_FILE}' { return [string]$Artifacts.Trace }
        '${CONFIG_FILE}' { return [string]$Artifacts.Config }
        '${STDOUT_FILE}' { return [string]$Artifacts.Stdout }
        '${STDERR_FILE}' { return [string]$Artifacts.Stderr }
    }
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return Resolve-RegressionPathWithinRoot -BasePath ([string]$Artifacts.ArtifactRoot) -RelativePath $Path -AllowedRoot ([string]$Artifacts.ArtifactRoot) -Label "checkpoint"
}

function Test-RegressionCheckpoint {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][object] $Checkpoint,
        [Parameter(Mandatory = $true)][hashtable] $Artifacts
    )

    $id = Get-RegressionCheckpointIdentity -Checkpoint $Checkpoint -Artifacts $Artifacts
    $required = Get-RegressionCheckpointRequired -Checkpoint $Checkpoint
    $group = "runtime-checkpoint"

    try {
        switch ([string]$Checkpoint.kind) {
            "stdoutRegex" { $textPath = [string]$Artifacts.Stdout }
            "stderrRegex" { $textPath = [string]$Artifacts.Stderr }
            "traceRegex" { $textPath = [string]$Artifacts.Trace }
            "fileExists" {
                $filePath = Resolve-RegressionCheckpointPath -Path ([string]$Checkpoint.path) -Artifacts $Artifacts
                if (Test-Path -LiteralPath $filePath -PathType Leaf) {
                    return New-RegressionCheck -Id $id -Group $group -Status pass -Required $required -Detail "file exists: $filePath" -Evidence @($filePath)
                }
                return New-RegressionCheck -Id $id -Group $group -Status fail -Required $required -Detail "required file is missing: $filePath" -Evidence @($filePath)
            }
            "image" {
                if (-not $Artifacts.ContainsKey("ImageRoot") -or -not $Artifacts.ContainsKey("ImageMetricsScript")) {
                    return New-RegressionCheck -Id $id -Group $group -Status error -Required $required -Detail "image checkpoint artifact map is incomplete"
                }
                $imagePath = Resolve-RegressionPathWithinRoot -BasePath ([string]$Artifacts.ImageRoot) -RelativePath ([string]$Checkpoint.artifact) -AllowedRoot ([string]$Artifacts.ImageRoot) -Label "image checkpoint"
                if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
                    return New-RegressionCheck -Id $id -Group $group -Status fail -Required $required -Detail "image checkpoint is missing: $imagePath" -Evidence @($imagePath)
                }
                $python = if ($Artifacts.ContainsKey("Python")) { [string]$Artifacts.Python } else { "python" }
                if ($null -eq (Get-Command $python -ErrorAction SilentlyContinue)) {
                    return New-RegressionCheck -Id $id -Group $group -Status error -Required $required -Detail "python command was not found: $python" -Evidence @($imagePath)
                }
                $metricsRoot = if ($Artifacts.ContainsKey("MetricsRoot")) { [string]$Artifacts.MetricsRoot } else { Join-Path ([string]$Artifacts.ArtifactRoot) "metrics" }
                New-Item -ItemType Directory -Path $metricsRoot -Force | Out-Null
                $safeName = $id -replace '[^A-Za-z0-9_.-]', '_'
                $metricsPath = Join-Path $metricsRoot ($safeName + ".json")
                $metricOutput = & $python ([string]$Artifacts.ImageMetricsScript) --image $imagePath --json $metricsPath 2>&1
                if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $metricsPath -PathType Leaf)) {
                    return New-RegressionCheck -Id $id -Group $group -Status fail -Required $required -Detail ("image metrics failed: " + (@($metricOutput) -join " ")) -Evidence @($imagePath)
                }
                $metrics = Get-Content -LiteralPath $metricsPath -Raw -Encoding UTF8 | ConvertFrom-Json
                if (-not $metrics.available) {
                    return New-RegressionCheck -Id $id -Group $group -Status warn -Required $required -Detail "image metrics are unavailable" -Evidence @($imagePath, $metricsPath)
                }
                $minimumWidth = if ($null -ne $Checkpoint.PSObject.Properties["minimumWidth"]) { [int]$Checkpoint.minimumWidth } else { 160 }
                $minimumHeight = if ($null -ne $Checkpoint.PSObject.Properties["minimumHeight"]) { [int]$Checkpoint.minimumHeight } else { 120 }
                $minimumNonBlackRatio = if ($null -ne $Checkpoint.PSObject.Properties["minimumNonBlackRatio"]) { [double]$Checkpoint.minimumNonBlackRatio } else { 0.002 }
                $metricStatus = if ($metrics.width -lt $minimumWidth -or $metrics.height -lt $minimumHeight -or $metrics.nonBlackRatio -lt $minimumNonBlackRatio) { "fail" } else { "pass" }
                return New-RegressionCheck -Id $id -Group $group -Status $metricStatus -Required $required -Detail ("image size={0}x{1}; nonBlackRatio={2}" -f $metrics.width, $metrics.height, $metrics.nonBlackRatio) -Evidence @($imagePath, $metricsPath)
            }
            default {
                return New-RegressionCheck -Id $id -Group $group -Status error -Required $required -Detail "unsupported checkpoint kind: $($Checkpoint.kind)"
            }
        }

        if (-not (Test-Path -LiteralPath $textPath -PathType Leaf)) {
            return New-RegressionCheck -Id $id -Group $group -Status fail -Required $required -Detail "checkpoint text file is missing: $textPath" -Evidence @($textPath)
        }
        $text = Get-Content -LiteralPath $textPath -Raw -Encoding UTF8
        $minimumMatches = [int]$Checkpoint.minimumMatches
        $matchCount = [regex]::Matches($text, [string]$Checkpoint.pattern).Count
        $status = if ($matchCount -ge $minimumMatches) { "pass" } else { "fail" }
        return New-RegressionCheck -Id $id -Group $group -Status $status -Required $required -Detail ("regex matches={0}; required={1}; pattern={2}" -f $matchCount, $minimumMatches, $Checkpoint.pattern) -Evidence @($textPath)
    }
    catch {
        return New-RegressionCheck -Id $id -Group $group -Status error -Required $required -Detail $_.Exception.Message
    }
}

Export-ModuleMember -Function @(
    "New-RegressionCheck",
    "Invoke-RegressionStage",
    "Get-RegressionAggregateStatus",
    "Get-RegressionExitCode",
    "Read-VerificationMatrix",
    "Test-VerificationMatrix",
    "Test-ArtifactSet",
    "New-BaselineManifest",
    "Test-BaselineManifest",
    "Test-RegressionCheckpoint",
    "New-RegressionRunSummary",
    "Write-RegressionReports"
)
