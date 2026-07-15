param(
    [ValidateSet("SelfTest", "InitBaseline", "Quick", "Full", "Step18")]
    [string] $Mode = "Quick",

    [string] $BaselineCommit = "44ec265659d2aa666cbf7546e36e4dde471d54ba",
    [string] $ArtifactsRoot,
    [string] $CurrentBuildDir,
    [string] $BaselineWorktree,
    [switch] $SkipBuild,
    [switch] $SkipRuntimeSmoke,
    [switch] $KeepTemp
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..\..")).Path
$scenarioRoot = Join-Path $scriptRoot "scenarios"
$helperRoot = Join-Path $scriptRoot "helpers"
$imageMetrics = Join-Path $helperRoot "image_metrics.py"
$selfTest = Join-Path $helperRoot "Run-CelestiaCompatSelfTest.ps1"
$verificationMatrixPath = Join-Path $scriptRoot "verification-matrix.json"

Import-Module (Join-Path $helperRoot "CelestiaCompatRegression.psm1") -Force

$vsdev = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat"
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ctest = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
$vcpkgToolchain = "D:\WorkSpace\Codex\_tools\vcpkg\scripts\buildsystems\vcpkg.cmake"

function Write-Step {
    param([string] $Message)
    Write-Host "[celestia-compat] $Message"
}

function Get-WorkspaceRoot {
    param([string] $Root)

    $normalized = [System.IO.Path]::GetFullPath($Root)
    $marker = [System.IO.Path]::DirectorySeparatorChar + ".worktrees" + [System.IO.Path]::DirectorySeparatorChar
    $index = $normalized.IndexOf($marker, [System.StringComparison]::OrdinalIgnoreCase)
    if ($index -ge 0) {
        return $normalized.Substring(0, $index)
    }

    return Split-Path -Parent $normalized
}

function Get-GitText {
    param(
        [string] $Root,
        [string[]] $Arguments
    )

    $output = & git -C $Root @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git command failed: git -C $Root $($Arguments -join ' ')"
    }

    return ($output | Out-String).Trim()
}

function Get-ShortCommit {
    param([string] $Root)
    return Get-GitText $Root @("rev-parse", "--short", "HEAD")
}

function Resolve-RegressionDefaults {
    $workspaceRoot = Get-WorkspaceRoot $repoRoot

    if ([string]::IsNullOrWhiteSpace($ArtifactsRoot)) {
        $script:ArtifactsRoot = Join-Path $workspaceRoot ".regression-artifacts\Celestia"
    }

    if ([string]::IsNullOrWhiteSpace($CurrentBuildDir)) {
        $script:CurrentBuildDir = Join-Path $repoRoot "build-mvc-sdl-rel"
    }

    if ([string]::IsNullOrWhiteSpace($BaselineWorktree)) {
        $script:BaselineWorktree = Join-Path $workspaceRoot ".worktrees\celestia-compat-baseline-44ec265"
    }
}

function New-Directory {
    param([string] $Path)
    New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Invoke-External {
    param(
        [string] $FilePath,
        [string[]] $ArgumentList,
        [string] $WorkingDirectory = $repoRoot,
        [string] $LogPath
    )

    Write-Step "$FilePath $($ArgumentList -join ' ')"
    $stdoutTemp = [System.IO.Path]::GetTempFileName()
    $stderrTemp = [System.IO.Path]::GetTempFileName()

    try {
        $startParams = @{
            FilePath = $FilePath
            ArgumentList = $ArgumentList
            WorkingDirectory = $WorkingDirectory
            RedirectStandardOutput = $stdoutTemp
            RedirectStandardError = $stderrTemp
            PassThru = $true
            Wait = $true
        }

        $process = Start-Process @startParams
        $exitCode = $process.ExitCode
        $stdout = Get-Content -LiteralPath $stdoutTemp -Raw
        $stderr = Get-Content -LiteralPath $stderrTemp -Raw
        $output = @()
        if (-not [string]::IsNullOrEmpty($stdout)) {
            $output += $stdout
        }
        if (-not [string]::IsNullOrEmpty($stderr)) {
            $output += $stderr
        }
    } finally {
        Remove-Item -LiteralPath $stdoutTemp, $stderrTemp -Force -ErrorAction SilentlyContinue
    }

    if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
        New-Directory (Split-Path -Parent $LogPath)
        ($output -join [Environment]::NewLine) | Out-File -FilePath $LogPath -Encoding utf8
    }

    if ($exitCode -ne 0) {
        $text = ($output -join [Environment]::NewLine).Trim()
        throw "Command failed with exit code ${exitCode}: $FilePath $($ArgumentList -join ' ')`n$text"
    }

    return $output
}

function Invoke-VsCommand {
    param(
        [string] $Command,
        [string] $LogPath
    )

    foreach ($required in @($vsdev, $cmake, $ctest)) {
        if (-not (Test-Path -LiteralPath $required)) {
            throw "Required Visual Studio tool was not found: $required"
        }
    }

    $cmd = 'call "{0}" -arch=x64 -host_arch=x64 >NUL && {1}' -f $vsdev, $Command
    Invoke-External -FilePath "cmd.exe" -ArgumentList @("/d", "/c", $cmd) -WorkingDirectory $repoRoot -LogPath $LogPath | Out-Null
}

function Invoke-CMakeAndCTest {
    param(
        [string] $SourceRoot,
        [string] $BuildDir,
        [string] $LogRoot,
        [string[]] $Targets = @("unit", "celestia-sdl"),
        [switch] $SkipCTest
    )

    Invoke-External -FilePath "git" -ArgumentList @("-C", $SourceRoot, "submodule", "update", "--init", "--recursive", "thirdparty/imgui", "thirdparty/miniaudio") -LogPath (Join-Path $LogRoot "submodule.log") | Out-Null
    New-Directory $BuildDir

    if ((Test-Path -LiteralPath (Join-Path $BuildDir "CMakeCache.txt")) -and
        (-not (Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja")))) {
        Remove-Item -LiteralPath (Join-Path $BuildDir "CMakeCache.txt") -Force
        Remove-Item -LiteralPath (Join-Path $BuildDir "CMakeFiles") -Recurse -Force -ErrorAction SilentlyContinue
    }

    if ((-not (Test-Path -LiteralPath (Join-Path $BuildDir "CMakeCache.txt"))) -or
        (-not (Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja")))) {
        $toolchainOptions = ""
        if (Test-Path -LiteralPath $vcpkgToolchain) {
            $toolchainOptions = ' -DCMAKE_TOOLCHAIN_FILE="{0}" -DVCPKG_TARGET_TRIPLET=x64-windows' -f $vcpkgToolchain
        }

        $configure = '"{0}" -S "{1}" -B "{2}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL=ON -DENABLE_QT6=OFF -DENABLE_WIN=OFF -DENABLE_NLS=OFF -DENABLE_CELX=OFF -DENABLE_TOOLS=OFF{3}' -f $cmake, $SourceRoot, $BuildDir, $toolchainOptions
        Invoke-VsCommand -Command $configure -LogPath (Join-Path $LogRoot "configure.log")
    }

    $build = '"{0}" --build "{1}" --config Release --target {2}' -f $cmake, $BuildDir, ($Targets -join " ")
    Invoke-VsCommand -Command $build -LogPath (Join-Path $LogRoot "build.log")

    if (-not $SkipCTest) {
        $test = '"{0}" --test-dir "{1}" -C Release --output-on-failure' -f $ctest, $BuildDir
        Invoke-VsCommand -Command $test -LogPath (Join-Path $LogRoot "ctest.log")
    }
}

function Invoke-MvcScans {
    param([string] $LogRoot)

    $scanLog = Join-Path $LogRoot "scan_mvc_dependencies.log"
    Invoke-External -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", (Join-Path $repoRoot "tools\mvc\scan_mvc_dependencies.ps1")) -LogPath $scanLog | Out-Null

    $cmakeScanLog = Join-Path $LogRoot "scan_cmake_targets.log"
    Invoke-External -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", (Join-Path $repoRoot "tools\mvc\scan_cmake_targets.ps1")) -LogPath $cmakeScanLog | Out-Null

    $modelAdapterBoundaryLog = Join-Path $LogRoot "test_mvc_model_adapter_boundary_clean.log"
    Invoke-External -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", (Join-Path $repoRoot "test\scripts\test_mvc_model_adapter_boundary_clean.ps1")) -LogPath $modelAdapterBoundaryLog | Out-Null
}

function Get-SdlExecutable {
    param([string] $BuildDir)

    $candidates = @(
        (Join-Path $BuildDir "src\celestia\sdl\celestia-sdl.exe"),
        (Join-Path $BuildDir "run-full\celestia-sdl.exe"),
        (Join-Path $BuildDir "celestia-sdl.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Could not find celestia-sdl.exe under build directory: $BuildDir"
}

function Get-ContentRoot {
    param([string] $BuildDir)

    $workspaceRoot = Get-WorkspaceRoot $repoRoot
    $candidates = @(
        (Join-Path $BuildDir "run-full"),
        $BuildDir,
        (Join-Path $CurrentBuildDir "run-full"),
        $CurrentBuildDir,
        (Join-Path $workspaceRoot "Celestia\build-mvc-sdl-rel\run-full")
    )

    foreach ($candidate in $candidates) {
        if ((Test-Path -LiteralPath (Join-Path $candidate "celestia.cfg")) -and
            (Test-Path -LiteralPath (Join-Path $candidate "data"))) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Could not find Celestia runtime content root under build directory: $BuildDir"
}

function ConvertTo-CelPath {
    param([string] $Path)
    return ([System.IO.Path]::GetFullPath($Path)).Replace("\", "/")
}

function New-DirectoryAlias {
    param(
        [string] $Source,
        [string] $Destination
    )

    if (Test-Path -LiteralPath $Destination) {
        return
    }

    $isWindows = $env:OS -like "Windows*"
    if ($isWindows) {
        $linkCommand = 'mklink /J "{0}" "{1}"' -f $Destination, $Source
        & cmd.exe /d /c $linkCommand | Out-Null
        if ($LASTEXITCODE -eq 0) {
            return
        }
    } else {
        try {
            New-Item -ItemType SymbolicLink -Path $Destination -Target $Source | Out-Null
            return
        } catch {
        }
    }

    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function New-ScenarioDataRoot {
    param(
        [string] $ContentRoot,
        [string] $BuildDir,
        [string] $RunRoot,
        [string] $ScenarioName,
        [string] $ScenarioText,
        [string] $ScreenshotDir
    )

    $tempRoot = Join-Path $RunRoot ("temp\data-" + $ScenarioName)
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-ScenarioDataRoot -DataRoot $tempRoot -RunRoot $RunRoot
    }
    New-Directory $tempRoot

    foreach ($item in Get-ChildItem -LiteralPath $ContentRoot -Force) {
        $destination = Join-Path $tempRoot $item.Name
        if ($item.PSIsContainer) {
            New-DirectoryAlias -Source $item.FullName -Destination $destination
        } elseif ($item.Name -ne "celestia.cfg" -and $item.Extension -notin @(".dll", ".exe", ".lib", ".pdb", ".exp")) {
            Copy-Item -LiteralPath $item.FullName -Destination $destination -Force
        }
    }

    foreach ($binaryRoot in @((Join-Path $BuildDir "src\celestia"), (Join-Path $BuildDir "src\celestia\sdl"))) {
        if (-not (Test-Path -LiteralPath $binaryRoot)) {
            continue
        }

        foreach ($dll in Get-ChildItem -LiteralPath $binaryRoot -Filter "*.dll" -File) {
            Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $tempRoot $dll.Name) -Force
        }
    }

    $scenarioFile = Join-Path $tempRoot "compat-scenario.cel"
    $ScenarioText | Out-File -FilePath $scenarioFile -Encoding utf8

    $cfgPath = Join-Path $ContentRoot "celestia.cfg"
    $cfg = Get-Content -LiteralPath $cfgPath -Raw
    $cfg = $cfg -replace 'InitScript\s+"[^"]+"', 'InitScript  "compat-scenario.cel"'
    $screenshotPath = ConvertTo-CelPath $ScreenshotDir
    $cfg = $cfg -replace 'ScriptScreenshotDirectory\s+"[^"]*"', ('ScriptScreenshotDirectory  "{0}"' -f $screenshotPath)
    $cfg | Out-File -FilePath (Join-Path $tempRoot "celestia.cfg") -Encoding utf8

    return $tempRoot
}

function Join-ProcessArguments {
    param([string[]] $Arguments)

    $quoted = New-Object System.Collections.Generic.List[string]
    foreach ($argument in $Arguments) {
        if ($null -eq $argument) {
            continue
        }

        if ($argument -match '[\s"]') {
            $escaped = $argument.Replace('\', '\\').Replace('"', '\"')
            $quoted.Add('"' + $escaped + '"')
        } else {
            $quoted.Add($argument)
        }
    }

    return ($quoted -join " ")
}

function Remove-ScenarioDataRoot {
    param(
        [string] $DataRoot,
        [string] $RunRoot
    )

    $safeRoot = [System.IO.Path]::GetFullPath($RunRoot)
    $safeTemp = [System.IO.Path]::GetFullPath($DataRoot)
    if (-not $safeTemp.StartsWith($safeRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove temp root outside run root: $DataRoot"
    }

    if (-not (Test-Path -LiteralPath $DataRoot)) {
        return
    }

    foreach ($child in Get-ChildItem -LiteralPath $DataRoot -Force) {
        if ($child.PSIsContainer -and (($child.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)) {
            & cmd.exe /d /c ('rmdir "{0}"' -f $child.FullName) | Out-Null
            if ($LASTEXITCODE -ne 0) {
                Remove-Item -LiteralPath $child.FullName -Force
            }
        } else {
            Remove-Item -LiteralPath $child.FullName -Recurse -Force
        }
    }

    Remove-Item -LiteralPath $DataRoot -Force
}

function Invoke-ProcessWithTimeout {
    param(
        [string] $FilePath,
        [string[]] $ArgumentList,
        [string] $WorkingDirectory,
        [string] $StdoutPath,
        [string] $StderrPath,
        [int] $TimeoutSeconds = 60,
        [hashtable] $Environment = @{},
        [switch] $AllowNonZeroExit
    )

    New-Directory (Split-Path -Parent $StdoutPath)
    New-Directory (Split-Path -Parent $StderrPath)

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = Join-ProcessArguments $ArgumentList
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $false

    foreach ($key in $Environment.Keys) {
        $startInfo.EnvironmentVariables[$key] = [string] $Environment[$key]
    }

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo

    $started = $process.Start()
    if (-not $started) {
        throw "Failed to start process: $FilePath"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        if (-not $process.HasExited) {
            $process.Kill()
        }
        $process.WaitForExit()
        $stdoutTask.Result | Out-File -FilePath $StdoutPath -Encoding utf8
        $stderrTask.Result | Out-File -FilePath $StderrPath -Encoding utf8
        throw "Process timed out after $TimeoutSeconds seconds: $FilePath"
    }
    $process.WaitForExit()

    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    $stdout | Out-File -FilePath $StdoutPath -Encoding utf8
    $stderr | Out-File -FilePath $StderrPath -Encoding utf8

    $process.Refresh()
    $exitCode = $process.ExitCode
    if ($null -eq $exitCode) {
        throw "Process exit code was not reported: $FilePath"
    }

    if ($exitCode -ne 0 -and -not $AllowNonZeroExit) {
        throw "Process failed with exit code ${exitCode}: $FilePath`n$stderr"
    }
    if ($AllowNonZeroExit) {
        return [pscustomobject]@{
            ExitCode = $exitCode
            StdoutPath = $StdoutPath
            StderrPath = $StderrPath
        }
    }
}

function Get-Scenarios {
    $matrix = Read-VerificationMatrix -Path $verificationMatrixPath
    Test-VerificationMatrix `
        -Matrix $matrix `
        -MatrixPath $verificationMatrixPath `
        -RepoRoot $repoRoot `
        -ScenarioRoot $scenarioRoot | Out-Null

    return @($matrix.unifiedExeScenarios | ForEach-Object {
        $scriptPath = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot ([string]$_.script)))
        [pscustomobject]@{
            Id = [string]$_.id
            Name = [System.IO.Path]::GetFileName($scriptPath)
            FullName = $scriptPath
            Definition = $_
        }
    })
}

function Invoke-ImageMetrics {
    param(
        [string] $ImagePath,
        [string] $JsonPath
    )

    Invoke-External -FilePath "python" -ArgumentList @($imageMetrics, "--image", $ImagePath, "--json", $JsonPath) | Out-Null
    return Get-Content -LiteralPath $JsonPath -Raw | ConvertFrom-Json
}

function Invoke-ImageComparison {
    param(
        [string] $BaselineImage,
        [string] $CurrentImage,
        [string] $JsonPath
    )

    Invoke-External -FilePath "python" -ArgumentList @($imageMetrics, "--baseline", $BaselineImage, "--current", $CurrentImage, "--json", $JsonPath) | Out-Null
    return Get-Content -LiteralPath $JsonPath -Raw | ConvertFrom-Json
}

function Invoke-ContactSheet {
    param(
        [object[]] $Entries,
        [string] $OutputPath,
        [string] $JsonPath
    )

    $arguments = @($imageMetrics, "--contact-sheet", $OutputPath, "--json", $JsonPath)
    foreach ($entry in $Entries) {
        $arguments += @("--sheet-entry", ("{0}={1}" -f $entry.Label, $entry.Path))
    }

    Invoke-External -FilePath "python" -ArgumentList $arguments | Out-Null
}

function Get-MetricStatus {
    param([object] $Metrics)

    if (-not $Metrics.available) {
        return "warn"
    }

    if ($Metrics.nonBlackRatio -lt 0.002) {
        return "fail"
    }

    if ($Metrics.width -lt 160 -or $Metrics.height -lt 120) {
        return "fail"
    }

    return "pass"
}

function Get-ComparisonStatus {
    param([object] $Comparison)

    if (-not $Comparison.available) {
        return "warn"
    }

    if (-not $Comparison.comparison.sameDimensions) {
        return "fail"
    }

    if ($Comparison.current.nonBlackRatio -lt 0.002) {
        return "fail"
    }

    if ($Comparison.comparison.dHashHamming -gt 30) {
        return "warn"
    }

    if ($Comparison.comparison.averageColorDistance -gt 80) {
        return "warn"
    }

    return "pass"
}

function New-UnicodeText {
    param([int[]] $Codepoints)
    return -join ($Codepoints | ForEach-Object { [char] $_ })
}

function Invoke-ScreenshotSet {
    param(
        [string] $Label,
        [string] $BuildDir,
        [string] $RunRoot
    )

    $exe = Get-SdlExecutable $BuildDir
    $contentRoot = Get-ContentRoot $BuildDir
    $screenshotDir = Join-Path $RunRoot "screenshots\$Label"
    $metricsDir = Join-Path $RunRoot "metrics\$Label"
    $logDir = Join-Path $RunRoot "logs\$Label"
    New-Directory $screenshotDir
    New-Directory $metricsDir
    New-Directory $logDir

    $results = New-Object System.Collections.Generic.List[object]
    foreach ($scenario in Get-Scenarios) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($scenario.Name)
        $imagePath = Join-Path $screenshotDir ($name + ".png")
        $jsonPath = Join-Path $metricsDir ($name + ".json")
        $scenarioText = Get-Content -LiteralPath $scenario.FullName -Raw
        $scenarioText = $scenarioText.Replace('$CAPTURE_FILE$', (ConvertTo-CelPath $imagePath))

        $dataRoot = New-ScenarioDataRoot `
            -ContentRoot $contentRoot `
            -BuildDir $BuildDir `
            -RunRoot $RunRoot `
            -ScenarioName $name `
            -ScenarioText $scenarioText `
            -ScreenshotDir $screenshotDir

        Write-Step "capture $Label/$name"
        Invoke-ProcessWithTimeout `
            -FilePath $exe `
            -ArgumentList @() `
            -WorkingDirectory $dataRoot `
            -StdoutPath (Join-Path $logDir ($name + ".stdout.log")) `
            -StderrPath (Join-Path $logDir ($name + ".stderr.log")) `
            -TimeoutSeconds 90 `
            -Environment @{ CELESTIA_DATA_DIR = $dataRoot }

        if (-not (Test-Path -LiteralPath $imagePath)) {
            throw "Screenshot was not created: $imagePath"
        }

        $metrics = Invoke-ImageMetrics -ImagePath $imagePath -JsonPath $jsonPath
        $results.Add([pscustomobject]@{
            Scene = $name
            Image = $imagePath
            MetricsJson = $jsonPath
            Status = Get-MetricStatus $metrics
            Metrics = $metrics
        })

        if (-not $KeepTemp) {
            Remove-ScenarioDataRoot -DataRoot $dataRoot -RunRoot $RunRoot
        }
    }

    return $results
}

function Invoke-RuntimeSmoke {
    param(
        [string] $BuildDir,
        [string] $RunRoot
    )

    $matrix = Read-VerificationMatrix -Path $verificationMatrixPath
    Test-VerificationMatrix -Matrix $matrix -MatrixPath $verificationMatrixPath -RepoRoot $repoRoot -ScenarioRoot $scenarioRoot | Out-Null
    $exe = Get-SdlExecutable $BuildDir
    $logDir = Join-Path $RunRoot "logs\runtime-smoke"
    New-Directory $logDir

    $checks = New-Object System.Collections.Generic.List[object]
    foreach ($scenario in @($matrix.runtimeScenarios)) {
        $scenarioId = [string]$scenario.id
        $scenarioRunRoot = Join-Path $logDir $scenarioId
        New-Directory $scenarioRunRoot
        $configPath = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot ([string]$scenario.config)))
        $runtimeConfig = Join-Path $scenarioRunRoot "runtime.yaml"
        $traceName = $scenarioId + ".trace"
        $tracePath = Join-Path $scenarioRunRoot $traceName
        $configText = Get-Content -LiteralPath $configPath -Raw
        $configText = $configText -replace '(?m)^(\s*traceFile:\s*).+$', ('$1' + $traceName)
        $configText | Out-File -FilePath $runtimeConfig -Encoding utf8
        $checks.Add((New-RegressionCheck -Id ("runtime/{0}/config" -f $scenarioId) -Group "runtime" -Status pass -Required ([bool]$scenario.required) -Detail "runtime config prepared" -Evidence @($configPath, $runtimeConfig)))

        Write-Step "runtime smoke $scenarioId"
        $stdoutPath = Join-Path $scenarioRunRoot "stdout.log"
        $stderrPath = Join-Path $scenarioRunRoot "stderr.log"
        $processStatus = "fail"
        $processDetail = "runtime process did not complete"
        try {
            $processResult = Invoke-ProcessWithTimeout `
                -FilePath $exe `
                -ArgumentList @("--runtime-config", $runtimeConfig) `
                -WorkingDirectory $scenarioRunRoot `
                -StdoutPath $stdoutPath `
                -StderrPath $stderrPath `
                -TimeoutSeconds 30 `
                -AllowNonZeroExit
            if ($processResult.ExitCode -eq 0) {
                $processStatus = "pass"
                $processDetail = "child exit code=0"
            }
            else {
                $processDetail = "child exit code=$($processResult.ExitCode)"
            }
        }
        catch {
            $processDetail = $_.Exception.Message
        }
        $checks.Add((New-RegressionCheck -Id ("runtime/{0}/process" -f $scenarioId) -Group "runtime-process" -Status $processStatus -Required ([bool]$scenario.required) -Detail $processDetail -Evidence @($runtimeConfig, $stdoutPath, $stderrPath)))

        $scenarioCheckpoints = @($scenario.checkpoints) + @($scenario.imageCheckpoints)
        if ($processStatus -ne "pass") {
            foreach ($checkpoint in $scenarioCheckpoints) {
                $checks.Add((New-SkippedRegressionCheck -Id ("{0}/{1}" -f $scenarioId, $checkpoint.id) -Group "runtime-checkpoint" -Detail "runtime process failed" -Required ([bool]$checkpoint.required)))
            }
            continue
        }

        $artifactMap = @{
            ScenarioId = $scenarioId
            Config = $runtimeConfig
            Stdout = $stdoutPath
            Stderr = $stderrPath
            Trace = $tracePath
            ArtifactRoot = $scenarioRunRoot
            ImageRoot = $scenarioRunRoot
            MetricsRoot = (Join-Path $scenarioRunRoot "metrics")
            ImageMetricsScript = $imageMetrics
            Python = "python"
        }
        foreach ($checkpoint in $scenarioCheckpoints) {
            $checkpointResult = Test-RegressionCheckpoint -Checkpoint $checkpoint -Artifacts $artifactMap
            if ([string]$checkpoint.id -match "synthetic") {
                $checkpointResult.Detail = "synthetic model identity; " + $checkpointResult.Detail
            }
            $checks.Add($checkpointResult)
        }
    }
    return $checks.ToArray()
}

function Assert-NoResidualProcesses {
    $names = @("celestia-sdl", "celestia-model-host", "celestia-controller-host", "celestia-view-host", "celestia-view3d-host")
    $processes = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $names -contains $_.ProcessName })
    if ($processes.Count -gt 0) {
        $list = ($processes | ForEach-Object { $_.ProcessName + ":" + $_.Id }) -join ", "
        throw "Residual Celestia processes remain: $list"
    }
}

function New-RunContext {
    param([string] $RunMode)

    $timestamp = Get-Date -Format "yyyy-MM-dd-HHmmss"
    $commit = Get-ShortCommit $repoRoot
    $runId = "{0}-{1}-{2}" -f $timestamp, $commit, $RunMode.ToLowerInvariant()
    $runRoot = Join-Path $ArtifactsRoot ("runs\" + $runId)
    New-Directory $runRoot

    return [pscustomobject]@{
        RunId = $runId
        Timestamp = $timestamp
        Commit = $commit
        RunRoot = $runRoot
        LogRoot = (Join-Path $runRoot "logs")
    }
}

function New-SkippedRegressionCheck {
    param(
        [Parameter(Mandatory = $true)][string] $Id,
        [Parameter(Mandatory = $true)][string] $Group,
        [Parameter(Mandatory = $true)][string] $Detail,
        [bool] $Required = $true
    )

    return New-RegressionCheck -Id $Id -Group $Group -Status skipped -Required $Required -Detail $Detail
}

function Test-ChecksAllowContinuation {
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $Checks)
    return @($Checks | Where-Object { $_.Status -in @("fail", "error") }).Count -eq 0
}

function Invoke-BuildChecks {
    param(
        [Parameter(Mandatory = $true)][string] $SourceRoot,
        [Parameter(Mandatory = $true)][string] $BuildDir,
        [Parameter(Mandatory = $true)][string] $LogRoot,
        [string[]] $Targets = @("unit", "celestia-sdl"),
        [switch] $SkipCTest
    )

    $checks = New-Object System.Collections.Generic.List[object]
    New-Directory $LogRoot

    $toolsStage = Invoke-RegressionStage -Id "build/prerequisites" -Group "harness" -FailureStatus error -Action {
        foreach ($required in @($vsdev, $cmake, $ctest)) {
            if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
                throw "Required Visual Studio tool was not found: $required"
            }
        }
        if ($null -eq (Get-Command git -ErrorAction SilentlyContinue)) {
            throw "git command was not found"
        }
        if ($null -eq (Get-Command python -ErrorAction SilentlyContinue)) {
            throw "python command was not found"
        }
    }
    $checks.Add($toolsStage.Check)
    if ($toolsStage.Check.Status -ne "pass") {
        foreach ($id in @("build/submodules", "build/configure", "build/compile", "build/ctest")) {
            $checks.Add((New-SkippedRegressionCheck -Id $id -Group "build" -Detail "build prerequisites failed"))
        }
        return $checks.ToArray()
    }

    $submoduleStage = Invoke-RegressionStage -Id "build/submodules" -Group "build" -Action {
        Invoke-External -FilePath "git" -ArgumentList @("-C", $SourceRoot, "submodule", "update", "--init", "--recursive", "thirdparty/imgui", "thirdparty/miniaudio") -LogPath (Join-Path $LogRoot "submodule.log") | Out-Null
    }
    $checks.Add($submoduleStage.Check)
    if ($submoduleStage.Check.Status -ne "pass") {
        foreach ($id in @("build/configure", "build/compile", "build/ctest")) {
            $checks.Add((New-SkippedRegressionCheck -Id $id -Group "build" -Detail "submodule stage failed"))
        }
        return $checks.ToArray()
    }

    $configureStage = Invoke-RegressionStage -Id "build/configure" -Group "build" -Action {
        New-Directory $BuildDir
        $cachePath = Join-Path $BuildDir "CMakeCache.txt"
        if (Test-Path -LiteralPath $cachePath -PathType Leaf) {
            $cachedCompilers = @(Select-String -LiteralPath $cachePath -Pattern '^CMAKE_(C|CXX)_COMPILER:FILEPATH=' | ForEach-Object {
                ($_.Line -split '=', 2)[1]
            })
            if (@($cachedCompilers | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) }).Count -gt 0) {
                Remove-Item -LiteralPath $cachePath -Force
                Remove-Item -LiteralPath (Join-Path $BuildDir "CMakeFiles") -Recurse -Force -ErrorAction SilentlyContinue
                Remove-Item -LiteralPath (Join-Path $BuildDir "build.ninja") -Force -ErrorAction SilentlyContinue
            }
        }
        if ((Test-Path -LiteralPath $cachePath) -and
            (-not (Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja")))) {
            Remove-Item -LiteralPath $cachePath -Force
            Remove-Item -LiteralPath (Join-Path $BuildDir "CMakeFiles") -Recurse -Force -ErrorAction SilentlyContinue
        }
        if ((-not (Test-Path -LiteralPath $cachePath)) -or
            (-not (Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja")))) {
            $toolchainOptions = ""
            if (Test-Path -LiteralPath $vcpkgToolchain) {
                $toolchainOptions = ' -DCMAKE_TOOLCHAIN_FILE="{0}" -DVCPKG_TARGET_TRIPLET=x64-windows' -f $vcpkgToolchain
            }
            $configure = '"{0}" -S "{1}" -B "{2}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL=ON -DENABLE_QT6=OFF -DENABLE_WIN=OFF -DENABLE_NLS=OFF -DENABLE_CELX=OFF -DENABLE_TOOLS=OFF{3}' -f $cmake, $SourceRoot, $BuildDir, $toolchainOptions
            Invoke-VsCommand -Command $configure -LogPath (Join-Path $LogRoot "configure.log")
        }
    }
    $checks.Add($configureStage.Check)
    if ($configureStage.Check.Status -ne "pass") {
        $checks.Add((New-SkippedRegressionCheck -Id "build/compile" -Group "build" -Detail "configure stage failed"))
        $checks.Add((New-SkippedRegressionCheck -Id "build/ctest" -Group "build" -Detail "configure stage failed"))
        return $checks.ToArray()
    }

    $buildStage = Invoke-RegressionStage -Id "build/compile" -Group "build" -Action {
        $build = '"{0}" --build "{1}" --config Release --target {2}' -f $cmake, $BuildDir, ($Targets -join " ")
        Invoke-VsCommand -Command $build -LogPath (Join-Path $LogRoot "build.log")
    }
    $checks.Add($buildStage.Check)
    if ($buildStage.Check.Status -ne "pass") {
        $checks.Add((New-SkippedRegressionCheck -Id "build/ctest" -Group "build" -Detail "compile stage failed"))
        return $checks.ToArray()
    }

    if ($SkipCTest) {
        $checks.Add((New-SkippedRegressionCheck -Id "build/ctest" -Group "build" -Detail "not required for baseline capture" -Required $false))
    }
    else {
        $testStage = Invoke-RegressionStage -Id "build/ctest" -Group "build" -Action {
            $test = '"{0}" --test-dir "{1}" -C Release --output-on-failure' -f $ctest, $BuildDir
            Invoke-VsCommand -Command $test -LogPath (Join-Path $LogRoot "ctest.log")
        }
        $checks.Add($testStage.Check)
    }
    return $checks.ToArray()
}

function Invoke-MvcScanChecks {
    param([Parameter(Mandatory = $true)][string] $LogRoot)

    $definitions = @(
        @{ Id = "mvc/scan-dependencies"; Script = Join-Path $repoRoot "tools\mvc\scan_mvc_dependencies.ps1"; Log = "scan_mvc_dependencies.log" },
        @{ Id = "mvc/scan-cmake-targets"; Script = Join-Path $repoRoot "tools\mvc\scan_cmake_targets.ps1"; Log = "scan_cmake_targets.log" },
        @{ Id = "mvc/model-adapter-boundary"; Script = Join-Path $repoRoot "test\scripts\test_mvc_model_adapter_boundary_clean.ps1"; Log = "test_mvc_model_adapter_boundary_clean.log" }
    )
    foreach ($definition in $definitions) {
        $stage = Invoke-RegressionStage -Id $definition.Id -Group "mvc-scan" -Action {
            Invoke-External -FilePath "powershell" -ArgumentList @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $definition.Script) -LogPath (Join-Path $LogRoot $definition.Log) | Out-Null
        }
        $stage.Check
    }
}

function Convert-ScreenshotResultsToChecks {
    param([Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]] $Screens)
    foreach ($screen in $Screens) {
        New-RegressionCheck `
            -Id ("unified/{0}/final-image" -f $screen.Scene) `
            -Group "unified-image" `
            -Status $screen.Status `
            -Required $true `
            -Detail ("image metrics status={0}" -f $screen.Status) `
            -Evidence @($screen.Image, $screen.MetricsJson)
    }
}

function Invoke-RuntimeValidation {
    param(
        [Parameter(Mandatory = $true)][string] $BuildDir,
        [Parameter(Mandatory = $true)][string] $RunRoot
    )

    if ($SkipRuntimeSmoke) {
        $matrix = Read-VerificationMatrix -Path $verificationMatrixPath
        foreach ($scenario in @($matrix.runtimeScenarios)) {
            New-SkippedRegressionCheck -Id ("runtime/{0}" -f $scenario.id) -Group "runtime" -Detail "-SkipRuntimeSmoke was specified"
        }
        return
    }

    try {
        Invoke-RuntimeSmoke -BuildDir $BuildDir -RunRoot $RunRoot
    }
    catch {
        New-RegressionCheck -Id "runtime/harness" -Group "runtime" -Status error -Required $true -Detail $_.Exception.Message
    }
}

function Ensure-BaselineWorktree {
    if (-not (Test-Path -LiteralPath $BaselineWorktree)) {
        New-Directory (Split-Path -Parent $BaselineWorktree)
        Invoke-External -FilePath "git" -ArgumentList @("worktree", "add", "--detach", $BaselineWorktree, $BaselineCommit) | Out-Null
        return
    }

    $worktreeStatus = Get-GitText $BaselineWorktree @("status", "--short")
    if (-not [string]::IsNullOrWhiteSpace($worktreeStatus)) {
        throw "Baseline worktree is dirty: $BaselineWorktree"
    }
    Invoke-External -FilePath "git" -ArgumentList @("-C", $BaselineWorktree, "checkout", "--detach", $BaselineCommit) | Out-Null
}

function Get-BaselineBuildDir {
    return Join-Path $BaselineWorktree "build-compat-sdl-rel"
}

function Get-BaselineArtifactRoot {
    $short = $BaselineCommit.Substring(0, 7)
    return Join-Path $ArtifactsRoot ("baselines\" + $short)
}

function Get-BaselineScreenshots {
    $screenshotRoot = Join-Path (Get-BaselineArtifactRoot) "screenshots\baseline"
    if (-not (Test-Path -LiteralPath $screenshotRoot)) {
        return @()
    }
    return @(Get-ChildItem -LiteralPath $screenshotRoot -Filter "*.png" -File | Sort-Object Name)
}

function Get-BaselineExpectedImages {
    return @(Get-Scenarios | ForEach-Object {
        $scenario = $_
        @($scenario.Definition.checkpoints | Where-Object kind -eq "image" | ForEach-Object {
            [pscustomobject]@{
                scenarioId = $scenario.Id
                checkpointId = [string]$_.id
                relativePath = ("screenshots/baseline/" + [string]$_.artifact)
            }
        })
    })
}

function Invoke-BaselineValidation {
    param([Parameter(Mandatory = $true)][object] $Context)

    $baselineRoot = Get-BaselineArtifactRoot
    $manifestPath = Join-Path $baselineRoot "baseline-manifest.json"
    $expectedImages = Get-BaselineExpectedImages
    $checks = New-Object System.Collections.Generic.List[object]
    foreach ($check in @(Test-BaselineManifest -ManifestPath $manifestPath -BaselineCommit $BaselineCommit -BaselineRoot $baselineRoot -ExpectedImages $expectedImages)) {
        $checks.Add($check)
    }

    $metricsRoot = Join-Path $Context.RunRoot "metrics\baseline"
    New-Directory $metricsRoot
    foreach ($entry in $expectedImages) {
        $imagePath = Join-Path $baselineRoot $entry.relativePath
        if (-not (Test-Path -LiteralPath $imagePath -PathType Leaf)) {
            continue
        }
        $metricsPath = Join-Path $metricsRoot ("{0}-{1}.json" -f $entry.scenarioId, $entry.checkpointId)
        try {
            $metrics = Invoke-ImageMetrics -ImagePath $imagePath -JsonPath $metricsPath
            $metricStatus = Get-MetricStatus $metrics
            $checks.Add((New-RegressionCheck -Id ("baseline/image/{0}/{1}" -f $entry.scenarioId, $entry.checkpointId) -Group "baseline" -Status $metricStatus -Required $true -Detail ("baseline image metrics status={0}; size={1}x{2}; nonBlackRatio={3}" -f $metricStatus, $metrics.width, $metrics.height, $metrics.nonBlackRatio) -Evidence @($imagePath, $metricsPath)))
        }
        catch {
            $checks.Add((New-RegressionCheck -Id ("baseline/image/{0}/{1}" -f $entry.scenarioId, $entry.checkpointId) -Group "baseline" -Status fail -Required $true -Detail ("baseline image is unreadable: " + $_.Exception.Message) -Evidence @($imagePath)))
        }
    }

    $baselineStatus = Get-RegressionAggregateStatus -Checks $checks.ToArray()
    if ($baselineStatus -eq "pass") {
        $checks.Add((New-RegressionCheck -Id "baseline/valid" -Group "baseline" -Status pass -Required $true -Detail "Baseline is valid for the current verification matrix." -Evidence @($manifestPath)))
    }
    else {
        $checks.Add((New-RegressionCheck -Id "baseline/valid" -Group "baseline" -Status fail -Required $true -Detail "Baseline is not valid for the current verification matrix. Run -Mode InitBaseline explicitly." -Evidence @($manifestPath)))
    }
    return $checks.ToArray()
}

function Invoke-InitBaseline {
    param([Parameter(Mandatory = $true)][object] $Context)

    $checks = New-Object System.Collections.Generic.List[object]
    $worktreeStage = Invoke-RegressionStage -Id "baseline/worktree" -Group "baseline" -Action { Ensure-BaselineWorktree }
    $checks.Add($worktreeStage.Check)
    if ($worktreeStage.Check.Status -ne "pass") {
        $checks.Add((New-SkippedRegressionCheck -Id "baseline/build" -Group "baseline" -Detail "baseline worktree unavailable"))
        $checks.Add((New-SkippedRegressionCheck -Id "baseline/capture" -Group "baseline" -Detail "baseline worktree unavailable"))
        return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = @(); BaselineScreens = @(); Comparisons = @(); RuntimeScenarios = @() }
    }

    $baselineBuild = Get-BaselineBuildDir

    if (-not $SkipBuild) {
        $baselineBuildChecks = @(
            Invoke-BuildChecks -SourceRoot $BaselineWorktree -BuildDir $baselineBuild -LogRoot (Join-Path $Context.LogRoot "baseline-build") -Targets @("celestia-sdl") -SkipCTest
        )
        foreach ($check in $baselineBuildChecks) {
            $checks.Add($check)
        }
        if (-not (Test-ChecksAllowContinuation -Checks $baselineBuildChecks)) {
            $checks.Add((New-SkippedRegressionCheck -Id "baseline/capture-set" -Group "baseline" -Detail "baseline build failed"))
            $checks.Add((New-SkippedRegressionCheck -Id "baseline/manifest-create" -Group "baseline" -Detail "baseline build failed"))
            return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = @(); BaselineScreens = @(); Comparisons = @(); RuntimeScenarios = @() }
        }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "baseline/build" -Group "build" -Detail "-SkipBuild was specified"))
    }

    $baselineRoot = Get-BaselineArtifactRoot
    New-Directory $baselineRoot
    $manifestPath = Join-Path $baselineRoot "baseline-manifest.json"
    Remove-Item -LiteralPath $manifestPath -Force -ErrorAction SilentlyContinue
    $baselineScreenshotRoot = Join-Path $baselineRoot "screenshots\baseline"
    if (Test-Path -LiteralPath $baselineScreenshotRoot -PathType Container) {
        Get-ChildItem -LiteralPath $baselineScreenshotRoot -Filter "*.png" -File | Remove-Item -Force
    }
    $captureStage = Invoke-RegressionStage -Id "baseline/capture-set" -Group "baseline" -Action {
        Invoke-ScreenshotSet -Label "baseline" -BuildDir $baselineBuild -RunRoot $baselineRoot
    }
    $checks.Add($captureStage.Check)
    $screens = @()
    if ($captureStage.Check.Status -eq "pass") {
        $screens = @($captureStage.Value)
        foreach ($check in @(Convert-ScreenshotResultsToChecks -Screens $screens)) { $checks.Add($check) }
    }
    $imageChecks = @($checks | Where-Object Group -eq "unified-image")
    if ($captureStage.Check.Status -eq "pass" -and (Get-RegressionAggregateStatus -Checks $imageChecks) -eq "pass") {
        $manifestStage = Invoke-RegressionStage -Id "baseline/manifest-create" -Group "baseline" -Action {
            New-BaselineManifest `
                -BaselineCommit $BaselineCommit `
                -BaselineRoot $baselineRoot `
                -ImageEntries (Get-BaselineExpectedImages) `
                -OutputPath $manifestPath
        }
        $checks.Add($manifestStage.Check)
        if ($manifestStage.Check.Status -eq "pass") {
            foreach ($check in @(Test-BaselineManifest -ManifestPath $manifestPath -BaselineCommit $BaselineCommit -BaselineRoot $baselineRoot -ExpectedImages (Get-BaselineExpectedImages))) {
                $checks.Add($check)
            }
        }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "baseline/manifest-create" -Group "baseline" -Detail "baseline images did not all pass"))
    }
    return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = @(); BaselineScreens = $screens; Comparisons = @(); RuntimeScenarios = @() }
}

function Invoke-Quick {
    param([Parameter(Mandatory = $true)][object] $Context)

    $checks = New-Object System.Collections.Generic.List[object]
    if (-not $SkipBuild) {
        foreach ($check in @(Invoke-BuildChecks -SourceRoot $repoRoot -BuildDir $CurrentBuildDir -LogRoot (Join-Path $Context.LogRoot "current-build"))) { $checks.Add($check) }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "build/all" -Group "build" -Detail "-SkipBuild was specified"))
    }
    foreach ($check in @(Invoke-MvcScanChecks -LogRoot (Join-Path $Context.LogRoot "mvc-scans"))) { $checks.Add($check) }

    $screens = @()
    if (Test-ChecksAllowContinuation -Checks @($checks | Where-Object Group -in @("build", "harness"))) {
        foreach ($check in @(Invoke-RuntimeValidation -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot)) { $checks.Add($check) }
        $captureStage = Invoke-RegressionStage -Id "unified/capture-set" -Group "unified" -Action {
            Invoke-ScreenshotSet -Label "current" -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot
        }
        $checks.Add($captureStage.Check)
        if ($captureStage.Check.Status -eq "pass") {
            $screens = @($captureStage.Value)
            foreach ($check in @(Convert-ScreenshotResultsToChecks -Screens $screens)) { $checks.Add($check) }
        }
        $residualStage = Invoke-RegressionStage -Id "runtime/no-residual-processes" -Group "runtime" -Action { Assert-NoResidualProcesses }
        $checks.Add($residualStage.Check)
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "runtime/all" -Group "runtime" -Detail "build stage failed"))
        $checks.Add((New-SkippedRegressionCheck -Id "unified/all" -Group "unified" -Detail "build stage failed"))
    }
    return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = $screens; BaselineScreens = @(); Comparisons = @(); RuntimeScenarios = @() }
}

function Invoke-Full {
    param([Parameter(Mandatory = $true)][object] $Context)

    $checks = New-Object System.Collections.Generic.List[object]
    $baselineChecks = @(Invoke-BaselineValidation -Context $Context)
    foreach ($check in $baselineChecks) { $checks.Add($check) }
    $baselineValid = (Get-RegressionAggregateStatus -Checks $baselineChecks) -eq "pass"

    if (-not $SkipBuild) {
        foreach ($check in @(Invoke-BuildChecks -SourceRoot $repoRoot -BuildDir $CurrentBuildDir -LogRoot (Join-Path $Context.LogRoot "current-build"))) { $checks.Add($check) }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "build/all" -Group "build" -Detail "-SkipBuild was specified"))
    }
    foreach ($check in @(Invoke-MvcScanChecks -LogRoot (Join-Path $Context.LogRoot "mvc-scans"))) { $checks.Add($check) }

    $screens = @()
    if (Test-ChecksAllowContinuation -Checks @($checks | Where-Object Group -in @("build", "harness"))) {
        foreach ($check in @(Invoke-RuntimeValidation -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot)) { $checks.Add($check) }
        $captureStage = Invoke-RegressionStage -Id "unified/capture-set" -Group "unified" -Action {
            Invoke-ScreenshotSet -Label "current" -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot
        }
        $checks.Add($captureStage.Check)
        if ($captureStage.Check.Status -eq "pass") {
            $screens = @($captureStage.Value)
            foreach ($check in @(Convert-ScreenshotResultsToChecks -Screens $screens)) { $checks.Add($check) }
        }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "runtime/all" -Group "runtime" -Detail "build stage failed"))
        $checks.Add((New-SkippedRegressionCheck -Id "unified/all" -Group "unified" -Detail "build stage failed"))
    }

    $baselineRoot = Get-BaselineArtifactRoot
    $comparisonRoot = Join-Path $Context.RunRoot "comparisons"
    New-Directory $comparisonRoot
    $contactEntries = New-Object System.Collections.Generic.List[object]
    $comparisons = New-Object System.Collections.Generic.List[object]

    if ($baselineValid) {
        foreach ($current in $screens) {
            $baselineImage = Join-Path $baselineRoot ("screenshots\baseline\" + $current.Scene + ".png")
            $jsonPath = Join-Path $comparisonRoot ($current.Scene + ".json")
            $comparisonStage = Invoke-RegressionStage -Id ("comparison/{0}" -f $current.Scene) -Group "comparison" -Action {
                Invoke-ImageComparison -BaselineImage $baselineImage -CurrentImage $current.Image -JsonPath $jsonPath
            }
            if ($comparisonStage.Check.Status -ne "pass") {
                $checks.Add($comparisonStage.Check)
                continue
            }
            $comparison = $comparisonStage.Value
            $comparisonStatus = Get-ComparisonStatus $comparison
            $checks.Add((New-RegressionCheck -Id ("comparison/{0}/metrics" -f $current.Scene) -Group "comparison" -Status $comparisonStatus -Required $true -Detail ("dHash={0}; averageColorDistance={1}" -f $comparison.comparison.dHashHamming, $comparison.comparison.averageColorDistance) -Evidence @($jsonPath, $baselineImage, $current.Image)))
            $comparisons.Add([pscustomobject]@{ Scene = $current.Scene; Status = $comparisonStatus; Json = $jsonPath; Comparison = $comparison })
            $contactEntries.Add([pscustomobject]@{ Label = $current.Scene + "-baseline"; Path = $baselineImage })
            $contactEntries.Add([pscustomobject]@{ Label = $current.Scene + "-current"; Path = $current.Image })
        }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "comparison/all" -Group "comparison" -Detail "baseline validation failed"))
    }

    if ($contactEntries.Count -gt 0) {
        $contactSheet = Join-Path $Context.RunRoot "contact-sheet.png"
        $contactJson = Join-Path $Context.RunRoot "contact-sheet.json"
        $contactStage = Invoke-RegressionStage -Id "comparison/contact-sheet" -Group "comparison" -Action {
            Invoke-ContactSheet -Entries $contactEntries -OutputPath $contactSheet -JsonPath $contactJson
        }
        if ($contactStage.Check.Status -eq "pass") {
            $checks.Add((New-RegressionCheck -Id "comparison/contact-sheet" -Group "comparison" -Status pass -Required $true -Detail "contact sheet created" -Evidence @($contactSheet, $contactJson)))
        }
        else {
            $checks.Add($contactStage.Check)
        }
    }
    $residualStage = Invoke-RegressionStage -Id "runtime/no-residual-processes" -Group "runtime" -Action { Assert-NoResidualProcesses }
    $checks.Add($residualStage.Check)
    return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = $screens; BaselineScreens = @(Get-BaselineScreenshots); Comparisons = $comparisons.ToArray(); RuntimeScenarios = @() }
}

function Invoke-Step18 {
    param([Parameter(Mandatory = $true)][object] $Context)

    $checks = New-Object System.Collections.Generic.List[object]
    if (-not $SkipBuild) {
        foreach ($check in @(Invoke-BuildChecks -SourceRoot $repoRoot -BuildDir $CurrentBuildDir -LogRoot (Join-Path $Context.LogRoot "current-build"))) { $checks.Add($check) }
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "build/all" -Group "build" -Detail "-SkipBuild was specified"))
    }
    foreach ($check in @(Invoke-MvcScanChecks -LogRoot (Join-Path $Context.LogRoot "mvc-scans"))) { $checks.Add($check) }

    $screens = @()
    if (Test-ChecksAllowContinuation -Checks @($checks | Where-Object Group -in @("build", "harness"))) {
        foreach ($check in @(Invoke-RuntimeValidation -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot)) { $checks.Add($check) }
        $captureStage = Invoke-RegressionStage -Id "unified/capture-set" -Group "unified" -Action {
            Invoke-ScreenshotSet -Label "current" -BuildDir $CurrentBuildDir -RunRoot $Context.RunRoot
        }
        $checks.Add($captureStage.Check)
        if ($captureStage.Check.Status -eq "pass") {
            $screens = @($captureStage.Value)
            foreach ($check in @(Convert-ScreenshotResultsToChecks -Screens $screens)) { $checks.Add($check) }
        }
        $residualStage = Invoke-RegressionStage -Id "runtime/no-residual-processes" -Group "runtime" -Action { Assert-NoResidualProcesses }
        $checks.Add($residualStage.Check)
    }
    else {
        $checks.Add((New-SkippedRegressionCheck -Id "runtime/all" -Group "runtime" -Detail "build stage failed"))
        $checks.Add((New-SkippedRegressionCheck -Id "unified/all" -Group "unified" -Detail "build stage failed"))
    }
    return [pscustomobject]@{ Context = $Context; Checks = $checks.ToArray(); CurrentScreens = $screens; BaselineScreens = @(); Comparisons = @(); RuntimeScenarios = @() }
}

function Invoke-SelectedMode {
    param(
        [Parameter(Mandatory = $true)][string] $SelectedMode,
        [Parameter(Mandatory = $true)][object] $Context
    )

    switch ($SelectedMode) {
        "InitBaseline" { return Invoke-InitBaseline -Context $Context }
        "Quick" { return Invoke-Quick -Context $Context }
        "Full" { return Invoke-Full -Context $Context }
        "Step18" { return Invoke-Step18 -Context $Context }
        default { throw "Unsupported run mode in formal gate: $SelectedMode" }
    }
}

Resolve-RegressionDefaults
New-Directory $ArtifactsRoot

if ($Mode -eq "SelfTest") {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $selfTest
    exit $LASTEXITCODE
}

$context = New-RunContext -RunMode $Mode
$result = $null
try {
    $result = Invoke-SelectedMode -SelectedMode $Mode -Context $context
    $checks = @($result.Checks)
}
catch {
    $checks = @(
        New-RegressionCheck `
            -Id "harness/unhandled-exception" `
            -Group "harness" `
            -Status error `
            -Required $true `
            -Detail $_.Exception.Message
    )
}

$summary = New-RegressionRunSummary `
    -RunId $context.RunId `
    -Mode $Mode `
    -CurrentCommit (Get-GitText $repoRoot @("rev-parse", "HEAD")) `
    -BaselineCommit $BaselineCommit `
    -VerificationMatrix "tools/regression/verification-matrix.json" `
    -ArtifactRoot $context.RunRoot `
    -Checks $checks

try {
    $reports = Write-RegressionReports -Summary $summary -OutputRoot $context.RunRoot
}
catch {
    Write-Error "Failed to write regression reports: $($_.Exception.Message)"
    exit 3
}

Write-Step "status: $($summary.status); exit code: $($summary.exitCode)"
Write-Step "json report: $($reports.Json)"
Write-Step "markdown report: $($reports.Markdown)"
exit $summary.exitCode
