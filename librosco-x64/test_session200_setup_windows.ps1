# Session 200 full Setup / new DLL isolated regression test.
# GitHub-hosted ephemeral runner only; independent AppId; NEVER published.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$NewDll,
    [Parameter(Mandatory=$true)][string]$BinaryAuditor,
    [Parameter(Mandatory=$true)][string]$QtPrefix
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if($env:GITHUB_ACTIONS -ne 'true' -or
   $env:RUNNER_ENVIRONMENT -ne 'github-hosted' -or
   [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) {
    throw 'SAFETY: independent Setup test only allowed on disposable GitHub-hosted runners'
}
if([string]::IsNullOrWhiteSpace($env:GH_TOKEN)) {
    throw 'Read-only GITHUB_TOKEN is needed to fetch the private official Session200 release'
}
foreach($file in @($NewDll,$BinaryAuditor)) {
    if(-not(Test-Path -LiteralPath $file)) { throw "Missing test input: $file" }
}
$expectedDllHash=(Get-FileHash -LiteralPath $NewDll -Algorithm SHA256).Hash
$root=Join-Path $env:RUNNER_TEMP ('mems-session200-setup-sandbox-'+[guid]::NewGuid().ToString('N'))
$downloads=Join-Path $root 'original-release'
$seed=Join-Path $root 'original-200-extracted-via-installer'
$tested=Join-Path $root 'independent-test-setup-installed'
$output=Join-Path $root 'test-only-setup'
$iss=Join-Path $root 'independent-test.iss'
New-Item -ItemType Directory -Force $downloads,$seed,$output | Out-Null
function CheckExit([int]$code,[string]$stage) {
    if($code -ne 0) { throw "$stage failed: exit=$code" }
}
function Install-Inno([string]$exe,[string]$destination,[string]$stage) {
    $args=@('/LANG=french','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOICONS',("/DIR="+$destination))
    $p=Start-Process -FilePath $exe -ArgumentList $args -PassThru -Wait
    CheckExit $p.ExitCode $stage
}
function Uninstall-Inno([string]$location) {
    $uninstaller=Get-ChildItem -LiteralPath $location -File -Filter 'unins*.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if($uninstaller) {
        $p=Start-Process -FilePath $uninstaller.FullName -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -PassThru -Wait
        if($p.ExitCode -ne 0) { Write-Warning "Sandbox uninstall returned $($p.ExitCode)" }
    }
}
try {
    # Official release read-only: full actual 200 payload, not a partial rebuild.
    & gh release download mems-session-build-200 --repo mini56/ECU-Mems-Manager-Session --pattern 'ECU-Mems-Manager-Session-200-Setup.exe' --pattern 'SESSION-200-Setup.sha256' --dir $downloads --clobber
    CheckExit $LASTEXITCODE 'Fetch official Session200 Setup'
    $originalSetup=Join-Path $downloads 'ECU-Mems-Manager-Session-200-Setup.exe'
    $shaFile=Join-Path $downloads 'SESSION-200-Setup.sha256'
    if(-not(Test-Path $originalSetup) -or -not(Test-Path $shaFile)) { throw 'Official release payload missing' }
    if((Get-Item -LiteralPath $originalSetup).Length -lt 100MB) { throw 'Original setup unexpectedly small' }
    $match=[regex]::Match((Get-Content -LiteralPath $shaFile -Raw),'(?i)[a-f0-9]{64}')
    if(-not $match.Success -or (Get-FileHash -LiteralPath $originalSetup -Algorithm SHA256).Hash -ine $match.Value) {
        throw 'Published official Session200 Setup hash mismatch'
    }
    Write-Host 'SESSION200_OFFICIAL_RELEASE_SETUP_SHA256_PASS'
    # Original installer is installed only in the empty disposable GH runner.
    Install-Inno $originalSetup $seed 'Extract original Session200 release'
    $seedExe=Join-Path $seed 'ecu_mems_manager.exe'
    $seedDll=Join-Path $seed 'mems_manager_x64.dll'
    foreach($item in @($seedExe,$seedDll,
        (Join-Path $seed 'MEMSLibrary.dll'),
        (Join-Path $seed 'MEMSLibrary_Pack_001/knowledge.sqlite'),
        (Join-Path $seed 'database/reference/runtime_visual_catalog.json'),
        (Join-Path $seed 'platforms/qoffscreen.dll'),
        (Join-Path $seed 'translations/fr/fr_session162.json'))) {
        if(-not(Test-Path -LiteralPath $item)) { throw "Official release file missing: $item" }
    }
    $officialExeHash=(Get-FileHash -LiteralPath $seedExe -Algorithm SHA256).Hash
    $originalDllHash=(Get-FileHash -LiteralPath $seedDll -Algorithm SHA256).Hash
    Write-Host "SESSION200_ORIGINAL_EXE_SHA256=$officialExeHash"
    Write-Host "SESSION200_ORIGINAL_DLL_SHA256=$originalDllHash"

    # Stage the only runtime modification: REPLACE DLL in the ephemeral copy.
    Copy-Item -LiteralPath $NewDll -Destination $seedDll -Force
    if((Get-FileHash -LiteralPath $seedDll -Algorithm SHA256).Hash -ine $expectedDllHash) {
        throw 'New DLL was not copied exactly to the ephemeral release payload'
    }
    & python $BinaryAuditor --dll $seedDll --manager $seedExe
    CheckExit $LASTEXITCODE 'Check official 200 executable against new DLL'
    Write-Host 'SESSION200_ORIGINAL_RELEASE_EXE_NEW_DLL_ABI_PASS'

    # Build a REAL, separate Windows Setup with independent AppId, user-only
    # install, no desktop/menu icons, no update action, no publication.
    $setupScript=@"
#define TestAppName "MEMS SESSION 200 - NO RELEASE - NEW DLL TEST"
[Setup]
AppId={{A821BAE2-A341-48A1-BF70-3EE119975AAB}
AppName={#TestAppName}
AppVersion=200.0.0
AppPublisher=Internal DLL test only
DefaultDirName={localappdata}\ECU-MEMS-SESSION200-DLL-ISOLATED-TEST
DefaultGroupName={#TestAppName}
UsePreviousAppDir=no
UsePreviousGroup=no
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
OutputDir=$output
OutputBaseFilename=SESSION200-DLL-TEST-ONLY-NOT-FOR-DISTRIBUTION
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
CloseApplications=no
RestartApplications=no
[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
[Files]
Source: "$seed\*"; DestDir: "{app}"; Excludes: "unins*.exe,unins*.dat,unins*.msg"; Flags: ignoreversion recursesubdirs createallsubdirs
[INI]
Filename: "{app}\session-install-language.ini"; Section: "Installation"; Key: "Language"; String: "fr"; Languages: french
Filename: "{app}\session-install-language.ini"; Section: "Installation"; Key: "Language"; String: "en"; Languages: english
"@
    $setupScript | Set-Content -LiteralPath $iss -Encoding utf8
    $iscc=Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Inno Setup 6/ISCC.exe'
    if(-not(Test-Path -LiteralPath $iscc)) { throw "Inno Setup 6 not found: $iscc" }
    & $iscc $iss
    CheckExit $LASTEXITCODE 'Compile independent Setup'
    $testSetup=Join-Path $output 'SESSION200-DLL-TEST-ONLY-NOT-FOR-DISTRIBUTION.exe'
    if(-not(Test-Path $testSetup) -or (Get-Item -LiteralPath $testSetup).Length -lt 100MB) {
        throw 'Independent test Setup did not contain the full release'
    }
    Write-Host "SESSION200_ISOLATED_TEST_SETUP_SHA256=$((Get-FileHash -LiteralPath $testSetup -Algorithm SHA256).Hash)"
    Write-Host 'SESSION200_INDEPENDENT_NEW_DLL_SETUP_BUILD_PASS'

    # Install the actual new test Setup into a DIFFERENT empty directory.
    Install-Inno $testSetup $tested 'Install independent Session200 test Setup'
    $installedExe=Join-Path $tested 'ecu_mems_manager.exe'
    $installedDll=Join-Path $tested 'mems_manager_x64.dll'
    foreach($item in @($installedExe,$installedDll,
        (Join-Path $tested 'MEMSLibrary.dll'),
        (Join-Path $tested 'MEMSLibrary_Pack_001/knowledge.sqlite'),
        (Join-Path $tested 'platforms/qoffscreen.dll'),
        (Join-Path $tested 'translations/fr/fr_session162.json'))) {
        if(-not(Test-Path -LiteralPath $item)) { throw "Test Setup file missing: $item" }
    }
    if((Get-FileHash -LiteralPath $installedExe -Algorithm SHA256).Hash -ine $officialExeHash) {
        throw 'Installed application EXE differs from official Session200 release EXE'
    }
    if((Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash -ine $expectedDllHash) {
        throw 'Test Setup did not install the exact newly compiled DLL'
    }
    & python $BinaryAuditor --dll $installedDll --manager $installedExe
    CheckExit $LASTEXITCODE 'PE imports/exports of installed official 200 with new DLL'
    Write-Host 'SESSION200_INSTALLED_EXE_AND_NEW_DLL_SHA256_ABI_PASS'

    # Existing 16-page NATIVE UI test from official RELEASE 200 EXE, running
    # WITH our new DLL from its installed directory, without any ECU.
    $env:QT_QPA_PLATFORM='offscreen'
    $env:QT_OPENGL='software'
    $env:QT_PLUGIN_PATH=$tested
    $env:QT_QPA_PLATFORM_PLUGIN_PATH=Join-Path $tested 'platforms'
    $env:PATH=$tested+';'+(Join-Path $QtPrefix 'bin')+';'+$env:PATH
    $trace=Join-Path $tested 'session_ui_smoke_trace.log'
    $stderr=Join-Path $tested 'session_ui_smoke_stderr.log'
    $stdout=Join-Path $tested 'session_ui_smoke_stdout.log'
    foreach($file in @($trace,$stderr,$stdout)) {
        if(Test-Path $file) { Remove-Item -LiteralPath $file -Force }
    }
    $p=Start-Process -FilePath $installedExe -WorkingDirectory $tested -ArgumentList '--session-ui-smoke-test' -RedirectStandardError $stderr -RedirectStandardOutput $stdout -PassThru
    $finished=$p.WaitForExit(300000)
    foreach($file in @($trace,$stderr,$stdout)) {
        Write-Host "SESSION200_TEST_INSTALL_LOG=$file"
        if(Test-Path $file) { Get-Content -LiteralPath $file | Select-Object -Last 60 }
    }
    if(-not $finished) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        throw 'INSTALLED_SESSION200_NEW_DLL_UI_TIMEOUT_300S'
    }
    if($p.ExitCode -ne 0) { throw "INSTALLED_SESSION200_NEW_DLL_UI_FAIL=$($p.ExitCode)" }
    $traceText=Get-Content -LiteralPath $trace -Raw
    if(-not $traceText.Contains('Checks PASS') -or
       -not $traceText.Contains('Qt event loop returned: 0')) {
        throw '16-page UI regression smoke did not confirm success after installation'
    }
    Write-Host 'SESSION200_ISOLATED_SETUP_INSTALLED_16PAGE_UI_PASS'

    Uninstall-Inno $tested
    if((Test-Path -LiteralPath $installedExe) -or (Test-Path -LiteralPath $installedDll)) {
        throw 'Test Setup uninstall failed: executable/DLL still present'
    }
    Write-Host 'SESSION200_ISOLATED_SETUP_UNINSTALL_PASS'
    Write-Host 'SESSION200_NEW_DLL_COMPLETE_SETUP_TEST_PASS'
    Write-Host 'LIMIT: no real ECU, hardware mutations or real-world Windows configurations tested.'
}
finally {
    foreach($dir in @($tested,$seed)) {
        if(Test-Path -LiteralPath $dir) {
            try { Uninstall-Inno $dir }
            catch { Write-Warning "Disposable runner uninstall warning: $_" }
        }
    }
    if(Test-Path -LiteralPath $root) {
        try { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction Stop }
        catch { Write-Warning "Disposable runner file cleanup warning: $_" }
    }
}
