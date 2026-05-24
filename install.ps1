# Mac Language Installer for Windows
# Usage: irm macstudio.meme/install.ps1 | iex

$ErrorActionPreference = "Stop"

$Repo = "jona62/mac"
$InstallDir = if ($env:MAC_INSTALL_DIR) { $env:MAC_INSTALL_DIR } else { "$env:USERPROFILE\.mac" }
$Target = "mac-windows-x86_64"

# ── Banner ──
Write-Host ""
Write-Host "  +-----------------------------+" -ForegroundColor Magenta
Write-Host "  |  Mac - Meme as Code         |" -ForegroundColor Magenta
Write-Host "  +-----------------------------+" -ForegroundColor Magenta
Write-Host ""

if (Test-Path "$InstallDir\mac.exe") {
    Write-Host "  ~ Updating ($Target)" -ForegroundColor Yellow
} else {
    Write-Host "  > Installing ($Target)" -ForegroundColor Blue
}

# ── Fetch latest release ──
Write-Host "  * Fetching latest release" -ForegroundColor Cyan -NoNewline
$Release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest"
$Version = $Release.tag_name
$Asset = $Release.assets | Where-Object { $_.name -match $Target -and $_.name -match "\.zip$" }
Write-Host "`r  v Fetching latest release" -ForegroundColor Green

if (-not $Asset) {
    Write-Host "  x No release found for $Target" -ForegroundColor Red
    Write-Host "  Build from source: cmake -S . -B build && cmake --build build" -ForegroundColor DarkGray
    exit 1
}

Write-Host "  v Found $Version" -ForegroundColor Green

# ── Download ──
$TmpDir = New-Item -ItemType Directory -Path (Join-Path $env:TEMP "mac_install_$(Get-Random)")
$ZipPath = Join-Path $TmpDir "mac.zip"
Write-Host "  * Downloading binary" -ForegroundColor Cyan -NoNewline
Invoke-WebRequest -Uri $Asset.browser_download_url -OutFile $ZipPath
Write-Host "`r  v Downloading binary" -ForegroundColor Green

# ── Extract ──
Write-Host "  * Extracting archive" -ForegroundColor Cyan -NoNewline
Expand-Archive -Path $ZipPath -DestinationPath $TmpDir -Force
Write-Host "`r  v Extracting archive" -ForegroundColor Green

# ── Install ──
if (Test-Path $InstallDir) {
    Remove-Item -Recurse -Force $InstallDir
}
Move-Item (Join-Path $TmpDir $Target) $InstallDir
Remove-Item -Recurse -Force $TmpDir
Write-Host "  v Installed to $InstallDir" -ForegroundColor Green

# ── PATH check ──
$UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($UserPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$InstallDir;$UserPath", "User")
    $env:Path = "$InstallDir;$env:Path"
    Write-Host "  v Added to user PATH" -ForegroundColor Green
} else {
    Write-Host "  v Already in PATH" -ForegroundColor Green
}

# ── Done ──
Write-Host ""
Write-Host "  Ready!" -ForegroundColor Green -NoNewline
Write-Host " Run " -NoNewline
Write-Host "mac" -ForegroundColor Cyan -NoNewline
Write-Host " to start the REPL."
Write-Host ""
Write-Host '  |> print "Hello, Mac!"' -ForegroundColor DarkGray
Write-Host "  Hello, Mac!" -ForegroundColor DarkGray
Write-Host ""
