$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$scanner = Join-Path $repoRoot "tools\mvc\scan_mvc_boundary_debt.ps1"
if (-not (Test-Path $scanner)) {
    throw "missing MVC boundary debt scanner: ${scanner}"
}

$output = & powershell -NoProfile -ExecutionPolicy Bypass -File $scanner 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "MVC boundary debt scanner failed with exit code $LASTEXITCODE`n$($output -join "`n")"
}

$text = $output -join "`n"

if ($text -match "\[model->adapter\]") {
    throw "model->adapter findings must be cleared before Step21 can pass`n${text}"
}

if ($text -match "\[model->view\]") {
    throw "model->view findings regressed during Step21`n${text}"
}

if ($text -match "\[model-view-symbol\]") {
    throw "model-view-symbol findings regressed during Step21`n${text}"
}

"MVC model adapter boundary is clean"
