$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$violations = New-Object System.Collections.Generic.List[string]

function Add-Violation {
    param(
        [string] $Path,
        [int] $LineNumber,
        [string] $Message,
        [string] $Line
    )

    $relative = [System.IO.Path]::GetRelativePath($repoRoot, $Path)
    $violations.Add("${relative}:${LineNumber}: ${Message}: ${Line}")
}

function Scan-TextFiles {
    param(
        [string] $Root,
        [string[]] $Include = @("*.h", "*.hpp", "*.cpp", "*.cc", "*.cxx", "*.c")
    )

    if (-not (Test-Path $Root)) {
        return @()
    }

    Get-ChildItem -LiteralPath $Root -Recurse -File -Include $Include
}

$protocolRoot = Join-Path $repoRoot "src\celruntime\protocol"
foreach ($file in Scan-TextFiles $protocolRoot) {
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $file.FullName) {
        $lineNumber++
        if ($line -match '#include\s+[<"].*(SDL|OpenGL|celrender|celengine/view3d|celengine\\view3d).*') {
            Add-Violation $file.FullName $lineNumber "protocol layer includes renderer or platform view dependency" $line
        }
    }
}

$runtimeSources = @(
    "src\celruntime",
    "src\celestia\sdl"
)

foreach ($root in $runtimeSources) {
    foreach ($file in Scan-TextFiles (Join-Path $repoRoot $root)) {
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $file.FullName) {
            $lineNumber++
            if ($line -match 'stdio-files|runServeSmoke|sdl-step6-serve') {
                Add-Violation $file.FullName $lineNumber "removed Step6 file-script fallback remains in production source" $line
            }
        }
    }
}

$forwardingRoots = @(
    "src\celengine",
    "src\celrender",
    "src\celrender\gl"
)

foreach ($root in $forwardingRoots) {
    $fullRoot = Join-Path $repoRoot $root
    if (-not (Test-Path $fullRoot)) {
        continue
    }

    foreach ($header in Get-ChildItem -LiteralPath $fullRoot -File -Filter "*.h") {
        Add-Violation $header.FullName 1 "root forwarding header remains after MVC closure" $header.Name
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output "MVC dependency scan passed"
