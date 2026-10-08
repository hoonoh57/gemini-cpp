$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$workDir = "E:\2026\gemini\gemini-cpp"
Set-Location -Path $workDir

Write-Host "=========================================================="
Write-Host "[CHECK] Target Directory: $workDir"
Write-Host "=========================================================="

# 1. 필수 소스 파일 점검
$requiredFiles = @(
    "build.bat",
    "main.cpp",
    "Common.hpp",
    "ChartTypes.hpp",
    "ChartCoreRenderer.hpp",
    "CentralDataManager.hpp",
    "FormulaManagerDlg.hpp",
    "StrategyConditionDlg.hpp"
)

$missing = @()
foreach ($file in$requiredFiles) {
    if (-not (Test-Path $file)) {
        $missing +=$file
    }
}

if ($missing.Count -gt 0) {
    Write-Host "[ERROR] Missing files:"
    foreach ($m in$missing) {
        Write-Host " - $m"
    }
    exit 1
} else {
    Write-Host "[OK] All required header and source files exist."
}

# 2. MSVC x64 컴파일 환경 탐색
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvars64 =$null

if (Test-Path $vswhere) {
    $installPath = &$vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($installPath -and (Test-Path "$installPath\VC\Auxiliary\Build\vcvars64.bat")) {
        $vcvars64 = "$installPath\VC\Auxiliary\Build\vcvars64.bat"
    }
}

if (-not $vcvars64) {$candidateList = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    )
    foreach ($c in$candidateList) {
        if (Test-Path $c) {
            $vcvars64 =$c
            break
        }
    }
}

if (-not $vcvars64) {
    Write-Host "[ERROR] vcvars64.bat not found."
    exit 1
}
Write-Host "[OK] Detected MSVC Toolset: $vcvars64"

# 3. build.bat 실행 및 결과 수집
Write-Host "[RUN] Executing build.bat..."
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "cmd.exe"
$psi.Arguments = "/c `"`"$vcvars64`" >nul 2>&1 && build.bat`""
$psi.WorkingDirectory =$workDir
$psi.RedirectStandardOutput =$true
$psi.RedirectStandardError =$true
$psi.UseShellExecute =$false
$psi.StandardOutputEncoding = [System.Text.Encoding]::UTF8$psi.StandardErrorEncoding = [System.Text.Encoding]::UTF8

$p = [System.Diagnostics.Process]::Start($psi)$out = $p.StandardOutput.ReadToEnd()$err = $p.StandardError.ReadToEnd()$p.WaitForExit()

Write-Host "------------------- BUILD OUTPUT -------------------"
if ($out) { Write-Host$out.Trim() }
if ($err) { Write-Host$err.Trim() }
Write-Host "----------------------------------------------------"

# 4. 판정
if ($p.ExitCode -eq 0 -and (Test-Path "kiwoom_modular_app.exe")) {
    $exe = Get-Item "kiwoom_modular_app.exe"
    Write-Host "[SUCCESS] Build finished successfully."
    Write-Host " - Binary: $($exe.FullName)"
    Write-Host " - Size: $($exe.Length) bytes"
    Write-Host " - Modified: $($exe.LastWriteTime)"
} else {
    Write-Host "[FAILED] Build exit code: $($p.ExitCode)"
    exit $p.ExitCode
}
