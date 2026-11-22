# Run on Windows x64 with Visual Studio 2022 C++ build tools and CMake.
# Builds our DLL against an EXACT, temporary checkout of Session 197.
# Never edits or commits the MEMS Manager application sources.
[CmdletBinding()]
param(
    [string]$QtPrefix = 'C:/Qt/5.15.2/msvc2019_64',
    [switch]$DllOnly,
    [switch]$KeepWorktree
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Session197Commit = '9105d9838e3fd5f81feeb99c0481c454d5d6a5e4'
$ScriptFolder = $PSScriptRoot
$Repository = (Resolve-Path (Join-Path $ScriptFolder '..')).Path
$TempPath = Join-Path ([IO.Path]::GetTempPath()) ('mems-dll-session197-' + [guid]::NewGuid().ToString('N'))
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
    Write-Host "SOURCE: Session 197 exact commit $Session197Commit"
    Invoke-Checked 'git' @('-C',$Repository,'fetch','--no-tags','--depth=1','origin',$Session197Commit)
    Invoke-Checked 'git' @('-C',$Repository,'worktree','add','--detach',$TempPath,$Session197Commit)
    $IsWorktree = $true

    $SessionHeader = Join-Path $TempPath 'rosco.h'
    $ReferenceHeader = Join-Path $ScriptFolder 'reference-session197/rosco.h'
    $Actual = [IO.File]::ReadAllText($SessionHeader) -replace '\r\n?', ([string][char]10)
    $Reference = [IO.File]::ReadAllText($ReferenceHeader) -replace '\r\n?', ([string][char]10)
    if ($Actual -cne $Reference) {
        throw 'Session 197 reference header is not identical to the pinned 197 source.'
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
        Write-Host 'SESSION197_MANAGER_NOT_TESTED: DLL-only mode selected'
        return
    }

    if (-not (Test-Path (Join-Path $QtPrefix 'lib/cmake/Qt5/Qt5Config.cmake'))) {
        throw "Qt5 MSVC2019 x64 is not installed at $QtPrefix. DLL tested, Manager197 not tested."
    }
    $env:PATH = (Join-Path $QtPrefix 'bin') + ';' + $env:PATH
    $env:QT_QPA_PLATFORM = 'offscreen'
    $ManagerBuild = Join-Path $TempPath '_build-manager197'
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
        throw 'Cannot locate compiled Session197 application or its bundled DLL.'
    }
    # The DLL is normally built in a different CMake subdirectory than the
    # Manager EXE. Stage it next to the EXE in the throwaway test folder,
    # rather than mistakenly claiming the installed Session 197 uses it.
    $StagedDll = Join-Path $AppBinary.DirectoryName 'mems_manager_x64.dll'
    Copy-Item -LiteralPath $ManagerDll.FullName -Destination $StagedDll -Force
    if (-not (Test-Path $StagedDll)) {
        throw 'Cannot stage the DLL next to the temporary Session197 executable.'
    }
    Invoke-Checked 'python' @(
        (Join-Path $TemporaryDllFolder 'validate_session197_binaries.py'),
        '--dll',$ManagerDll.FullName,'--manager',$AppBinary.FullName
    )
    Write-Host 'SESSION197_LINK_AND_PE_IMPORTS_PASS: the real Session 197 links to our new DLL'
    Write-Host 'NOTE: No actual ECU test and no Manager installed-Setup/UI runtime test was performed.'
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
