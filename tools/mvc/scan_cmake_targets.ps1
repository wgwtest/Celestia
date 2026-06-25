$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$violations = New-Object System.Collections.Generic.List[string]

function Add-Violation {
    param([string] $Message)
    $violations.Add($Message)
}

function Read-RepoText {
    param([string] $RelativePath)
    $path = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path $path)) {
        Add-Violation "missing required CMake file: ${RelativePath}"
        return ""
    }

    return Get-Content -LiteralPath $path -Raw
}

$celengine = Read-RepoText "src\celengine\CMakeLists.txt"
$runtime = Read-RepoText "src\celruntime\CMakeLists.txt"
$views = Read-RepoText "src\celestia\viewproviders\CMakeLists.txt"
$sdl = Read-RepoText "src\celestia\sdl\CMakeLists.txt"

if ($celengine -notmatch 'add_library\(celestia_model OBJECT') {
    Add-Violation "celestia_model object target is missing"
}

if ($celengine -match 'target_link_libraries\(celestia_model') {
    Add-Violation "celestia_model must not link view, controller, renderer, or adapter targets"
}

if ($views -match 'target_link_libraries\(celestia_viewprovider_debug2d PRIVATE celestia_runtime celestia_view3d') {
    Add-Violation "debug2D view provider must not link the OpenGL3D view target"
}

foreach ($required in @(
    'assembly/runtimeassemblyconfig.cpp',
    'assembly/runtimeassemblyrunner.cpp',
    'transport/transportfactory.cpp',
    'transport/localsockettransport.cpp',
    'dataplane/dataplaneref.cpp',
    'dataplane/sharedmemorydataplane.cpp',
    'add_executable\(celestia-view3d-host'
)) {
    if ($runtime -notmatch [regex]::Escape($required).Replace('\\\(', '\(')) {
        Add-Violation "celestia_runtime CMake is missing required Step10/Step11 entry: ${required}"
    }
}

if ($sdl -notmatch 'target_link_libraries\(celestia-sdl PRIVATE imgui celestia\)') {
    Add-Violation "celestia-sdl should link through the application shell and celestia runtime facade"
}

if (($runtime + $sdl) -match 'stdio-files|runServeSmoke|sdl-step6-serve') {
    Add-Violation "removed stdio-files fallback still appears in runtime or SDL CMake-adjacent source"
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output "MVC CMake target scan passed"
