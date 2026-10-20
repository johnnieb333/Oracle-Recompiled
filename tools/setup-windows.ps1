$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
$root = Split-Path -Parent $PSScriptRoot
$env:ORACLE_SETUP_ROOT = $root

function Choose-Rom([string]$title, [string]$expected) {
    while ($true) {
        $dialog = New-Object System.Windows.Forms.OpenFileDialog
        $dialog.Title = $title
        $dialog.Filter = 'Game Boy Color ROM (*.gbc)|*.gbc|All files (*.*)|*.*'
        $dialog.CheckFileExists = $true
        $result = $dialog.ShowDialog()
        $file = $dialog.FileName
        $dialog.Dispose()
        if ($result -ne [System.Windows.Forms.DialogResult]::OK) { return $null }
        if ((Get-Item -LiteralPath $file).Length -eq 1048576 -and
            (Get-FileHash -LiteralPath $file -Algorithm SHA1).Hash.ToLowerInvariant() -eq $expected) {
            return $file
        }
        [System.Windows.Forms.MessageBox]::Show(
            'That is not the supported USA ROM for this game. Please select another file.',
            'Oracle Setup', 'OK', 'Warning') | Out-Null
    }
}

try {
    if (!(Test-Path -LiteralPath (Join-Path $root 'tools\prepare_game.py'))) {
        throw 'Keep Setup-Oracle.bat and tools together in the source project.'
    }
    $answer = [System.Windows.Forms.MessageBox]::Show(
        "Choose your Ages and Seasons USA ROMs. Setup will then build both games and the launcher automatically.`n`nThe first build can take 20-40 minutes or longer. Existing saves are preserved. This preview requires an installed MSYS2 MINGW64 toolchain.",
        'Oracle Setup', 'OKCancel', 'Information')
    if ($answer -ne [System.Windows.Forms.DialogResult]::OK) { exit 0 }

    $msysRoot = 'C:\msys64'
    if (!(Test-Path -LiteralPath (Join-Path $msysRoot 'usr\bin\bash.exe'))) {
        $folder = New-Object System.Windows.Forms.FolderBrowserDialog
        $folder.Description = 'Select your installed MSYS2 folder (normally C:\msys64).'
        if ($folder.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { $folder.Dispose(); exit 0 }
        $msysRoot = $folder.SelectedPath
        $folder.Dispose()
    }
    $bash = Join-Path $msysRoot 'usr\bin\bash.exe'
    if (!(Test-Path -LiteralPath $bash)) {
        throw 'MSYS2 was not found. Install MSYS2 and the prerequisites in EASY-SETUP.txt, then run Setup-Oracle.bat again.'
    }
    $ages = Choose-Rom 'Select your Oracle of Ages USA ROM' '880374fb978b18af4aa529e2e32f7ffb4d7dd2f4'
    if (!$ages) { exit 0 }
    $seasons = Choose-Rom 'Select your Oracle of Seasons USA ROM' 'ba1268290fb2b1b70505d2d7b5825fc8a4816a4b'
    if (!$seasons) { exit 0 }
    $env:ORACLE_SETUP_AGES = $ages
    $env:ORACLE_SETUP_SEASONS = $seasons
    $env:MSYSTEM = 'MINGW64'
    $env:CHERE_INVOKING = '1'
    Write-Host 'Building Oracle Recompiled. Leave this window open.'
    # Pass paths through environment variables, never interpolate them as shell code.
    $buildScript = (Join-Path $PSScriptRoot 'setup-build.sh').Replace('\', '/')
    & $bash --login $buildScript
    if ($LASTEXITCODE -ne 0) { throw 'Setup stopped. The build window above shows the error. No saves were removed.' }
    $launcher = Join-Path $root 'build-launcher\bin\oracle-launcher.exe'
    if (!(Test-Path -LiteralPath $launcher)) { throw 'The launcher executable was not produced.' }
    [System.Windows.Forms.MessageBox]::Show(
        'Setup finished. Use Start-Oracle.bat next time to open the launcher.',
        'Oracle Setup', 'OK', 'Information') | Out-Null
    Start-Process -FilePath $launcher
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Oracle Setup', 'OK', 'Error') | Out-Null
    exit 1
} finally {
    Remove-Item Env:\ORACLE_SETUP_AGES -ErrorAction SilentlyContinue
    Remove-Item Env:\ORACLE_SETUP_SEASONS -ErrorAction SilentlyContinue
}
