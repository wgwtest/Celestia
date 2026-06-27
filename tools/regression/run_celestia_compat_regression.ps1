param(
    [ValidateSet("SelfTest", "InitBaseline", "Quick", "Full")]
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
        [hashtable] $Environment = @{}
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

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill()
        throw "Process timed out after $TimeoutSeconds seconds: $FilePath"
    }

    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $stdout | Out-File -FilePath $StdoutPath -Encoding utf8
    $stderr | Out-File -FilePath $StderrPath -Encoding utf8

    $process.Refresh()
    $exitCode = $process.ExitCode
    if ($null -eq $exitCode) {
        throw "Process exit code was not reported: $FilePath"
    }

    if ($exitCode -ne 0) {
        throw "Process failed with exit code ${exitCode}: $FilePath`n$stderr"
    }
}

function Get-Scenarios {
    if (-not (Test-Path -LiteralPath $scenarioRoot)) {
        throw "Scenario directory is missing: $scenarioRoot"
    }

    $scenarios = @(Get-ChildItem -LiteralPath $scenarioRoot -Filter "*.cel" -File | Sort-Object Name)
    if ($scenarios.Count -ne 8) {
        throw "Expected 8 scenarios, found $($scenarios.Count)"
    }

    return $scenarios
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

    if ($SkipRuntimeSmoke) {
        return @([pscustomobject]@{ Name = "runtime-smoke"; Status = "skipped"; Detail = "-SkipRuntimeSmoke" })
    }

    $exe = Get-SdlExecutable $BuildDir
    $logDir = Join-Path $RunRoot "logs\runtime-smoke"
    New-Directory $logDir

    $configs = @(
        "runtime-2d-stdio.yaml",
        "runtime-3d-stdio.yaml",
        "runtime-2d-local-socket.yaml",
        "runtime-3d-local-socket.yaml",
        "runtime-switch-2d-to-3d-local-socket.yaml",
        "runtime-switch-3d-to-2d-local-socket.yaml"
    )

    $results = New-Object System.Collections.Generic.List[object]
    foreach ($configName in $configs) {
        $configPath = Join-Path $repoRoot ("DOC\CODEX_DOC\examples\" + $configName)
        if (-not (Test-Path -LiteralPath $configPath)) {
            $results.Add([pscustomobject]@{ Name = $configName; Status = "fail"; Detail = "missing config" })
            continue
        }

        $runtimeConfig = Join-Path $logDir $configName
        $traceName = [System.IO.Path]::GetFileNameWithoutExtension($configName) + ".trace"
        $configText = Get-Content -LiteralPath $configPath -Raw
        $configText = $configText -replace '(?m)^(\s*traceFile:\s*).+$', ('$1' + $traceName)
        $configText | Out-File -FilePath $runtimeConfig -Encoding utf8

        Write-Step "runtime smoke $configName"
        Invoke-ProcessWithTimeout `
            -FilePath $exe `
            -ArgumentList @("--runtime-config", $runtimeConfig) `
            -WorkingDirectory $logDir `
            -StdoutPath (Join-Path $logDir ($configName + ".stdout.log")) `
            -StderrPath (Join-Path $logDir ($configName + ".stderr.log")) `
            -TimeoutSeconds 30

        $results.Add([pscustomobject]@{ Name = $configName; Status = "pass"; Detail = $runtimeConfig })
    }

    return $results
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
    $runRoot = Join-Path $ArtifactsRoot ("runs\{0}-{1}-{2}" -f $timestamp, $commit, $RunMode.ToLowerInvariant())
    New-Directory $runRoot

    return [pscustomobject]@{
        Timestamp = $timestamp
        Commit = $commit
        RunRoot = $runRoot
        LogRoot = (Join-Path $runRoot "logs")
    }
}

function Write-Report {
    param(
        [object] $Context,
        [string] $RunMode,
        [object[]] $CurrentScreens = @(),
        [object[]] $BaselineScreens = @(),
        [object[]] $Comparisons = @(),
        [object[]] $RuntimeSmoke = @(),
        [string] $ExtraStatus = "pass"
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $currentCommit = Get-GitText $repoRoot @("rev-parse", "HEAD")
    $lines.Add("# Celestia Compatibility Regression Machine Report")
    $lines.Add("")
    $lines.Add(("- mode: ``{0}``" -f $RunMode))
    $lines.Add(("- timestamp: ``{0}``" -f $Context.Timestamp))
    $lines.Add(("- current commit: ``{0}``" -f $currentCommit))
    $lines.Add(("- baseline commit: ``{0}``" -f $BaselineCommit))
    $lines.Add(("- artifacts: ``{0}``" -f $Context.RunRoot))
    $lines.Add(("- status: ``{0}``" -f $ExtraStatus))
    $lines.Add("")

    if ($CurrentScreens.Count -gt 0) {
        $lines.Add("## Current Screenshots")
        $lines.Add("")
        $lines.Add("| Scene | Status | Image | Metrics |")
        $lines.Add("| --- | --- | --- | --- |")
        foreach ($item in $CurrentScreens) {
            $lines.Add(("| ``{0}`` | ``{1}`` | ``{2}`` | ``{3}`` |" -f $item.Scene, $item.Status, $item.Image, $item.MetricsJson))
        }
        $lines.Add("")
    }

    if ($BaselineScreens.Count -gt 0) {
        $lines.Add("## Baseline Screenshots")
        $lines.Add("")
        $lines.Add("| Scene | Status | Image | Metrics |")
        $lines.Add("| --- | --- | --- | --- |")
        foreach ($item in $BaselineScreens) {
            $lines.Add(("| ``{0}`` | ``{1}`` | ``{2}`` | ``{3}`` |" -f $item.Scene, $item.Status, $item.Image, $item.MetricsJson))
        }
        $lines.Add("")
    }

    if ($Comparisons.Count -gt 0) {
        $lines.Add("## Baseline Current Comparison")
        $lines.Add("")
        $lines.Add("| Scene | Status | dHash Hamming | Average Color Distance | JSON |")
        $lines.Add("| --- | --- | --- | --- | --- |")
        foreach ($item in $Comparisons) {
            $lines.Add(("| ``{0}`` | ``{1}`` | ``{2}`` | ``{3}`` | ``{4}`` |" -f $item.Scene, $item.Status, $item.Comparison.comparison.dHashHamming, $item.Comparison.comparison.averageColorDistance, $item.Json))
        }
        $lines.Add("")
    }

    if ($RuntimeSmoke.Count -gt 0) {
        $lines.Add("## Runtime Smoke")
        $lines.Add("")
        $lines.Add("| Config | Status | Detail |")
        $lines.Add("| --- | --- | --- |")
        foreach ($item in $RuntimeSmoke) {
            $lines.Add(("| ``{0}`` | ``{1}`` | ``{2}`` |" -f $item.Name, $item.Status, $item.Detail))
        }
        $lines.Add("")
    }

    $lines.Add("## Claim Boundary")
    $lines.Add("")
    $lines.Add('A passing `Full` run supports only this claim:')
    $lines.Add("")
    $lines.Add('```text')
    $lines.Add("No visible regression was found in the ordinary unified SDL exe / in-process main path compared with the fixed pre-MVC baseline covered by these scenes.")
    $lines.Add('```')
    $lines.Add("")
    $lines.Add("It does not prove exhaustive Celestia feature parity, pixel-perfect rendering, or Qt/Win32 frontend parity.")

    $reportPath = Join-Path $Context.RunRoot "machine-report.md"
    $lines | Out-File -FilePath $reportPath -Encoding utf8

    if ($RunMode -eq "Full") {
        $testDocDir = "06_" + (New-UnicodeText @(0x6D4B, 0x8BD5, 0x6587, 0x6863))
        $machineRecordDir = "03_" + (New-UnicodeText @(0x673A, 0x6D4B, 0x8BB0, 0x5F55))
        $docReportRoot = Join-Path (Join-Path (Join-Path $repoRoot "DOC\CODEX_DOC") $testDocDir) $machineRecordDir
        New-Directory $docReportRoot
        $docReport = Join-Path $docReportRoot ($Context.Timestamp + "-Celestia-compat-regression-machine-report.md")
        Copy-Item -LiteralPath $reportPath -Destination $docReport -Force
    }

    return $reportPath
}

function Ensure-BaselineWorktree {
    if (-not (Test-Path -LiteralPath $BaselineWorktree)) {
        New-Directory (Split-Path -Parent $BaselineWorktree)
        Invoke-External -FilePath "git" -ArgumentList @("worktree", "add", "--detach", $BaselineWorktree, $BaselineCommit) | Out-Null
        return
    }

    $status = Get-GitText $BaselineWorktree @("status", "--short")
    if (-not [string]::IsNullOrWhiteSpace($status)) {
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

function Invoke-InitBaseline {
    $context = New-RunContext "InitBaseline"
    Ensure-BaselineWorktree
    $baselineBuild = Get-BaselineBuildDir

    if (-not $SkipBuild) {
        Invoke-CMakeAndCTest -SourceRoot $BaselineWorktree -BuildDir $baselineBuild -LogRoot (Join-Path $context.LogRoot "baseline-build") -Targets @("celestia-sdl") -SkipCTest
    }

    $baselineRoot = Get-BaselineArtifactRoot
    New-Directory $baselineRoot
    $screens = Invoke-ScreenshotSet -Label "baseline" -BuildDir $baselineBuild -RunRoot $baselineRoot
    $report = Write-Report -Context $context -RunMode "InitBaseline" -BaselineScreens $screens
    Write-Step "baseline report: $report"
}

function Get-BaselineScreenshots {
    $baselineRoot = Get-BaselineArtifactRoot
    $screenshotRoot = Join-Path $baselineRoot "screenshots\baseline"
    if (-not (Test-Path -LiteralPath $screenshotRoot)) {
        return @()
    }
    return @(Get-ChildItem -LiteralPath $screenshotRoot -Filter "*.png" -File | Sort-Object Name)
}

function Ensure-BaselineScreenshots {
    $screens = Get-BaselineScreenshots
    if ($screens.Count -eq 8) {
        return
    }

    Write-Step "baseline screenshots missing or incomplete; running InitBaseline first"
    Invoke-InitBaseline
}

function Invoke-Quick {
    $context = New-RunContext "Quick"
    if (-not $SkipBuild) {
        Invoke-CMakeAndCTest -SourceRoot $repoRoot -BuildDir $CurrentBuildDir -LogRoot (Join-Path $context.LogRoot "current-build")
    }

    Invoke-MvcScans -LogRoot (Join-Path $context.LogRoot "mvc-scans")
    $runtimeSmoke = Invoke-RuntimeSmoke -BuildDir $CurrentBuildDir -RunRoot $context.RunRoot
    $screens = Invoke-ScreenshotSet -Label "current" -BuildDir $CurrentBuildDir -RunRoot $context.RunRoot
    Assert-NoResidualProcesses
    $report = Write-Report -Context $context -RunMode "Quick" -CurrentScreens $screens -RuntimeSmoke $runtimeSmoke
    Write-Step "quick report: $report"
}

function Invoke-Full {
    Ensure-BaselineScreenshots
    $context = New-RunContext "Full"

    if (-not $SkipBuild) {
        Invoke-CMakeAndCTest -SourceRoot $repoRoot -BuildDir $CurrentBuildDir -LogRoot (Join-Path $context.LogRoot "current-build")
    }

    Invoke-MvcScans -LogRoot (Join-Path $context.LogRoot "mvc-scans")
    $runtimeSmoke = Invoke-RuntimeSmoke -BuildDir $CurrentBuildDir -RunRoot $context.RunRoot
    $screens = Invoke-ScreenshotSet -Label "current" -BuildDir $CurrentBuildDir -RunRoot $context.RunRoot

    $baselineRoot = Get-BaselineArtifactRoot
    $comparisonRoot = Join-Path $context.RunRoot "comparisons"
    New-Directory $comparisonRoot
    $contactEntries = New-Object System.Collections.Generic.List[object]
    $comparisons = New-Object System.Collections.Generic.List[object]

    foreach ($current in $screens) {
        $baselineImage = Join-Path $baselineRoot ("screenshots\baseline\" + $current.Scene + ".png")
        $jsonPath = Join-Path $comparisonRoot ($current.Scene + ".json")
        $comparison = Invoke-ImageComparison -BaselineImage $baselineImage -CurrentImage $current.Image -JsonPath $jsonPath
        $comparisons.Add([pscustomobject]@{
            Scene = $current.Scene
            Status = Get-ComparisonStatus $comparison
            Json = $jsonPath
            Comparison = $comparison
        })
        $contactEntries.Add([pscustomobject]@{ Label = $current.Scene + "-baseline"; Path = $baselineImage })
        $contactEntries.Add([pscustomobject]@{ Label = $current.Scene + "-current"; Path = $current.Image })
    }

    Invoke-ContactSheet -Entries $contactEntries -OutputPath (Join-Path $context.RunRoot "contact-sheet.png") -JsonPath (Join-Path $context.RunRoot "contact-sheet.json")
    Assert-NoResidualProcesses

    $status = "pass"
    if (@($screens | Where-Object { $_.Status -eq "fail" }).Count -gt 0 -or
        @($comparisons | Where-Object { $_.Status -eq "fail" }).Count -gt 0 -or
        @($runtimeSmoke | Where-Object { $_.Status -eq "fail" }).Count -gt 0) {
        $status = "fail"
    } elseif (@($screens | Where-Object { $_.Status -eq "warn" }).Count -gt 0 -or
              @($comparisons | Where-Object { $_.Status -eq "warn" }).Count -gt 0 -or
              @($runtimeSmoke | Where-Object { $_.Status -eq "warn" }).Count -gt 0) {
        $status = "warn"
    }

    $report = Write-Report -Context $context -RunMode "Full" -CurrentScreens $screens -Comparisons $comparisons -RuntimeSmoke $runtimeSmoke -ExtraStatus $status
    Write-Step "full report: $report"
}

Resolve-RegressionDefaults
New-Directory $ArtifactsRoot

switch ($Mode) {
    "SelfTest" {
        Invoke-External -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", $selfTest) | Out-Null
    }
    "InitBaseline" {
        Invoke-InitBaseline
    }
    "Quick" {
        Invoke-Quick
    }
    "Full" {
        Invoke-Full
    }
}
