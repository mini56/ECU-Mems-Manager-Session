# Run on Windows x64 with Visual Studio 2022 C++ build tools and CMake.
# Builds our DLL against an EXACT, temporary checkout of Session 200.
# Never edits or commits the MEMS Manager application sources.
[CmdletBinding()]
param(
    [string]$QtPrefix = 'C:/Qt/5.15.2/msvc2019_64',
    [switch]$DllOnly,
    [switch]$KeepWorktree,
    [switch]$SetupSmoke
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Session200Commit = '3737dffa3e30e0b9167bda34fa661958b757cd28'
$ScriptFolder = $PSScriptRoot
$Repository = (Resolve-Path (Join-Path $ScriptFolder '..')).Path
$TempPath = Join-Path ([IO.Path]::GetTempPath()) ('mems-dll-session200-' + [guid]::NewGuid().ToString('N'))
$IsWorktree = $false
function Invoke-Checked([string]$Program,[string[]]$Arguments) {
    Write-Host ('RUN ' + $Program + ' ' + ($Arguments -join ' '))
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program exited $LASTEXITCODE"
    }
}
if (-not (Test-Path (Join-Path $Repository '.git'))) {
    throw 'Run this script from the dll-manager Git working tree; the Git metadata is required.'
}
try {
    Write-Host "SOURCE: Session 200 exact commit $Session200Commit"
    Invoke-Checked 'git' @('-C',$Repository,'fetch','--no-tags','--depth=1','origin',$Session200Commit)
    Invoke-Checked 'git' @('-C',$Repository,'worktree','add','--detach',$TempPath,$Session200Commit)
    $IsWorktree = $true

    $SessionHeader = Join-Path $TempPath 'rosco.h'
    $ReferenceHeader = Join-Path $ScriptFolder 'reference-session200/rosco.h'
    $Actual = [IO.File]::ReadAllText($SessionHeader) -replace '\r\n?', ([string][char]10)
    $Reference = [IO.File]::ReadAllText($ReferenceHeader) -replace '\r\n?', ([string][char]10)
    if ($Actual -cne $Reference) {
        throw 'Session 200 reference header is not identical to the pinned 200 source.'
    }

    # Replacing the DLL in this THROWAWAY worktree does not touch the Session
    # application, original DLL branch, any installed version or any ECU.
    $TemporaryDllFolder = Join-Path $TempPath 'librosco-x64'
    if (Test-Path $TemporaryDllFolder) {
        Remove-Item -LiteralPath $TemporaryDllFolder -Recurse -Force
    }
    Copy-Item -LiteralPath $ScriptFolder -Destination $TemporaryDllFolder -Recurse -Force

    Invoke-Checked 'python' @((Join-Path $TemporaryDllFolder 'validate_protocol_guards.py'))

    $DllBuild = Join-Path $TempPath '_build-dll-only'
    Invoke-Checked 'cmake' @('-S',$TemporaryDllFolder,'-B',$DllBuild,'-A','x64')
    Invoke-Checked 'cmake' @('--build',$DllBuild,'--config','Release','--parallel','4')
    Invoke-Checked 'ctest' @('--test-dir',$DllBuild,'-C','Release','--output-on-failure')
    $DllBinary = Join-Path $DllBuild 'Release/mems_manager_x64.dll'
    if (-not (Test-Path $DllBinary)) {
        throw 'No compiled mems_manager_x64.dll after x64 build.'
    }
    Write-Host 'DLL_BUILD_AND_CTEST_PASS: tests run on compiled Windows x64 DLL'

    if ($DllOnly) {
        Write-Host 'SESSION200_MANAGER_NOT_TESTED: DLL-only mode selected'
        return
    }

    if (-not (Test-Path (Join-Path $QtPrefix 'lib/cmake/Qt5/Qt5Config.cmake'))) {
        throw "Qt5 MSVC2019 x64 is not installed at $QtPrefix. DLL tested, Manager200 not tested."
    }
    $env:PATH = (Join-Path $QtPrefix 'bin') + ';' + $env:PATH
    $env:QT_QPA_PLATFORM = 'offscreen'
    $ManagerBuild = Join-Path $TempPath '_build-manager200'
    Invoke-Checked 'cmake' @(
        '-S',$TempPath,'-B',$ManagerBuild,'-A','x64',
        "-DCMAKE_PREFIX_PATH=$QtPrefix",
        '-DMEMS_USE_BUNDLED_LIBROSCO_X64=ON',
        '-DMEMS_RUN_REFERENCE_SELFTEST_POST_BUILD=OFF',
        '-DMEMS_USE_ONNX_GENAI=OFF'
    )
    Invoke-Checked 'cmake' @(
        '--build',$ManagerBuild,'--config','Release',
        '--target','ecu_mems_manager','--parallel','4'
    )

    $AppBinary = Get-ChildItem -LiteralPath $ManagerBuild -Recurse -File -Filter 'ecu_mems_manager.exe' | Select-Object -First 1
    $ManagerDll = Get-ChildItem -LiteralPath (Join-Path $ManagerBuild 'librosco-x64') -Recurse -File -Filter 'mems_manager_x64.dll' | Select-Object -First 1
    if (-not $AppBinary -or -not $ManagerDll) {
        throw 'Cannot locate compiled Session200 application or its bundled DLL.'
    }
    # The DLL is normally built in a different CMake subdirectory than the
    # Manager EXE. Stage it next to the EXE in the throwaway test folder,
    # rather than mistakenly claiming the installed Session 200 uses it.
    $StagedDll = Join-Path $AppBinary.DirectoryName 'mems_manager_x64.dll'
    Copy-Item -LiteralPath $ManagerDll.FullName -Destination $StagedDll -Force
    if (-not (Test-Path $StagedDll)) {
        throw 'Cannot stage the DLL next to the temporary Session200 executable.'
    }
    Invoke-Checked 'python' @(
        (Join-Path $TemporaryDllFolder 'validate_session200_binaries.py'),
        '--dll',$ManagerDll.FullName,'--manager',$AppBinary.FullName
    )
    Write-Host 'SESSION200_LINK_AND_PE_IMPORTS_PASS: the real Session 200 links to our new DLL'

    # Run Session 200's existing native UI smoke mode in the isolated worktree.
    # No installer, production profile, active serial ports or ECU commands.
    $WinDeployQt = Join-Path $QtPrefix 'bin/windeployqt.exe'
    if (-not (Test-Path $WinDeployQt)) { throw "Qt runtime deployer missing: $WinDeployQt" }
    Invoke-Checked $WinDeployQt @('--release','--compiler-runtime','--no-translations',
                                 '--dir',$AppBinary.DirectoryName,$AppBinary.FullName)
    $QtOffscreen = Join-Path $QtPrefix 'plugins/platforms/qoffscreen.dll'
    if (-not (Test-Path $QtOffscreen)) { throw "Qt offscreen plugin missing: $QtOffscreen" }
    $Platforms = Join-Path $AppBinary.DirectoryName 'platforms'
    New-Item -ItemType Directory -Force $Platforms | Out-Null
    Copy-Item -LiteralPath $QtOffscreen -Destination (Join-Path $Platforms 'qoffscreen.dll') -Force
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_OPENGL = 'software'
    $env:QT_PLUGIN_PATH = $AppBinary.DirectoryName
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = $Platforms
    $env:PATH = $AppBinary.DirectoryName + ';' + $env:PATH
    $Trace = Join-Path $AppBinary.DirectoryName 'session_ui_smoke_trace.log'
    $Err = Join-Path $AppBinary.DirectoryName 'session_ui_smoke_stderr.log'
    $Out = Join-Path $AppBinary.DirectoryName 'session_ui_smoke_stdout.log'
    foreach ($p in @($Trace,$Err,$Out)) {
        if (Test-Path $p) { Remove-Item -LiteralPath $p -Force }
    }
    $Ui = Start-Process -FilePath $AppBinary.FullName -WorkingDirectory $AppBinary.DirectoryName -ArgumentList '--session-ui-smoke-test' -RedirectStandardError $Err -RedirectStandardOutput $Out -PassThru
    $Completed = $Ui.WaitForExit(300000)
    foreach ($p in @($Trace,$Err,$Out)) {
        Write-Host "SESSION200_UI_SMOKE_DIAGNOSTIC: $p"
        if (Test-Path $p) { Get-Content -LiteralPath $p | Select-Object -Last 110 }
        else { Write-Host 'Log not created' }
    }
    if (-not $Completed) {
        Stop-Process -Id $Ui.Id -Force -ErrorAction SilentlyContinue
        throw 'SESSION200_UI_SMOKE_TIMEOUT_300S'
    }
    if ($Ui.ExitCode -ne 0) { throw "SESSION200_UI_SMOKE_FAILED exit=$($Ui.ExitCode)" }
    Write-Host 'SESSION200_UI_SMOKE_WITH_NEW_DLL_PASS'
    if ($SetupSmoke) {
        # Real published Session 200 files + new DLL; isolated test-only Inno
        # AppId, on disposable GitHub-hosted runner, no release or MAJ.
        & (Join-Path $TemporaryDllFolder 'test_session200_setup_windows.ps1') -NewDll $ManagerDll.FullName -BinaryAuditor (Join-Path $TemporaryDllFolder 'validate_session200_binaries.py') -QtPrefix $QtPrefix
        if ($LASTEXITCODE -ne 0) { throw 'Independent Session200 Setup test returned an error' }
    }
    Write-Host 'NOTE: no physical ECU was tested; no Setup published or installed on a user machine.'

}
finally {
    if ($IsWorktree -and -not $KeepWorktree) {
        & git -C $Repository worktree remove --force $TempPath
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Temporary worktree cleanup failed: $TempPath"
        }
    }
    elseif ($KeepWorktree) {
        Write-Host "TEMPORARY_TEST_WORKTREE: $TempPath"
    }
}
