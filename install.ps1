$ErrorActionPreference = "Stop"

param(
    [string]$Prefix = "$env:LOCALAPPDATA\zwasm",
    [string]$Version = ""
)

$Repo = "clojurewasm/zwasm"
$BinDir = Join-Path $Prefix "bin"
$Artifact = "zwasm-windows-x86_64"

if (-not $Version) {
    $Release = Invoke-RestMethod "https://api.github.com/repos/$Repo/releases/latest"
    $Version = $Release.tag_name
    if (-not $Version) {
        throw "Could not determine latest version"
    }
}

$Url = "https://github.com/$Repo/releases/download/$Version/$Artifact.zip"
$TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("zwasm-install-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $TempDir | Out-Null

try {
    $ZipPath = Join-Path $TempDir "$Artifact.zip"
    Write-Host "Installing zwasm $Version (windows/x86_64)..."
    Invoke-WebRequest -Uri $Url -OutFile $ZipPath
    Expand-Archive -Path $ZipPath -DestinationPath $TempDir -Force

    New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
    Move-Item -Force (Join-Path $TempDir "zwasm.exe") (Join-Path $BinDir "zwasm.exe")

    Write-Host "Installed: $(Join-Path $BinDir 'zwasm.exe')"
    $UserPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $UserPath.Split(';').Contains($BinDir)) {
        Write-Host ""
        Write-Host "Add to your PATH:"
        Write-Host "  [Environment]::SetEnvironmentVariable('Path', `"$UserPath;$BinDir`", 'User')"
    }
}
finally {
    Remove-Item -Recurse -Force $TempDir -ErrorAction SilentlyContinue
}
