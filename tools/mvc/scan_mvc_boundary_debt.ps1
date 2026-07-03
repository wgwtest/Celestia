param(
    [string] $RepoRoot = "",
    [switch] $FailOnFindings
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Join-Path $PSScriptRoot "..\.."
}

$repoRootPath = (Resolve-Path $RepoRoot).Path
$findings = New-Object System.Collections.Generic.List[object]

function Get-RepoRelativePath {
    param([string] $Path)

    if ([System.IO.Path].GetMethod("GetRelativePath", [type[]]@([string], [string])) -ne $null) {
        return [System.IO.Path]::GetRelativePath($repoRootPath, $Path)
    }

    $root = $repoRootPath
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }

    $rootUri = New-Object System.Uri($root)
    $pathUri = New-Object System.Uri($Path)
    return [System.Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString()).Replace('/', [System.IO.Path]::DirectorySeparatorChar)
}

function Add-Finding {
    param(
        [string] $Category,
        [string] $Path,
        [int] $LineNumber,
        [string] $Message,
        [string] $Line
    )

    $relative = Get-RepoRelativePath $Path
    $findings.Add([pscustomobject]@{
        Category = $Category
        Path = $relative
        LineNumber = $LineNumber
        Message = $Message
        Line = $Line.Trim()
    })
}

function Get-SourceFiles {
    param([string] $RelativeRoot)

    $root = Join-Path $repoRootPath $RelativeRoot
    if (-not (Test-Path $root)) {
        return @()
    }

    Get-ChildItem -LiteralPath $root -Recurse -File -Include @("*.h", "*.hpp", "*.cpp", "*.cc", "*.cxx", "*.c")
}

function Scan-LineRule {
    param(
        [string] $RelativeRoot,
        [string] $Category,
        [string] $Pattern,
        [string] $Message
    )

    foreach ($file in Get-SourceFiles $RelativeRoot) {
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $file.FullName) {
            $lineNumber++
            if ($line -match $Pattern) {
                Add-Finding $Category $file.FullName $lineNumber $Message $line
            }
        }
    }
}

$includePrefix = '#include\s+[<"]'

Scan-LineRule "src\celengine\model" `
    "model->view" `
    ($includePrefix + '.*(celengine/view3d|celengine\\view3d|celrender|celrender\\).*') `
    "model source includes view or renderer dependency"

Scan-LineRule "src\celengine\model" `
    "model->adapter" `
    ($includePrefix + '.*(celengine/adapter|celengine\\adapter).*') `
    "model source includes adapter dependency"

Scan-LineRule "src\celengine\model" `
    "model-view-symbol" `
    '\b(ReferenceMark|MarkerRepresentation|MarkerList|CurvePlot|CurvePlotSample)\b' `
    "model source mentions view-facing symbol"

Scan-LineRule "src\celengine\controller" `
    "controller->view" `
    ($includePrefix + '.*(celengine/view3d|celengine\\view3d|celrender|celrender\\).*') `
    "controller source includes view or renderer dependency"

Scan-LineRule "src\celengine\adapter" `
    "adapter->runtime" `
    ($includePrefix + '.*(celruntime|celruntime\\).*') `
    "adapter source includes runtime dependency"

Scan-LineRule "src\celengine\adapter" `
    "adapter->view" `
    ($includePrefix + '.*(celengine/view3d|celengine\\view3d|celrender|celrender\\).*') `
    "adapter source includes view or renderer dependency"

Scan-LineRule "src\celruntime\model" `
    "runtime-model->view" `
    ($includePrefix + '.*(celengine/view3d|celengine\\view3d|celrender|celrender\\).*') `
    "runtime model source includes view or renderer dependency"

Scan-LineRule "src\celruntime\model" `
    "runtime-model->app" `
    ($includePrefix + '.*(celestia/|celestia\\).*') `
    "runtime model source includes application-layer dependency"

Scan-LineRule "src\celruntime\model" `
    "runtime-model-projection" `
    '\b(ViewFrame|SceneFrame|BodyRenderState|StarRenderState|OrbitRenderState|LabelRenderState)\b' `
    "runtime model source mentions view projection state"

if ($findings.Count -eq 0) {
    Write-Output "MVC boundary debt scan found 0 findings"
    exit 0
}

Write-Output ("MVC boundary debt scan report: {0} finding(s)" -f $findings.Count)

$findings |
    Sort-Object Category, Path, LineNumber |
    Group-Object Category |
    ForEach-Object {
        Write-Output ""
        Write-Output ("[{0}] {1} finding(s)" -f $_.Name, $_.Count)
        $_.Group | ForEach-Object {
            Write-Output ("{0}:{1}: {2}: {3}" -f $_.Path, $_.LineNumber, $_.Message, $_.Line)
        }
    }

if ($FailOnFindings) {
    exit 1
}
