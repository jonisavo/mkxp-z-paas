# vars.ps1 - Environment setup for mkxp-z Windows build

$DIR = Split-Path -Parent $MyInvocation.MyCommand.Path

# Determine the host architecture and set the prefix
$MKXPZ_HOST = & gcc -dumpmachine 2>$null
if ($MKXPZ_HOST -match "i686") {
    $MKXPZ_PREFIX = "mingw"
} else {
    $MKXPZ_PREFIX = "mingw64"
}

$BuildDir = Join-Path $DIR "build-$MKXPZ_PREFIX"

# Set environment variables
$env:LDFLAGS = "-L$BuildDir/lib -L$BuildDir/bin"
$env:CFLAGS = "-I$BuildDir/include"
$env:CXXFLAGS = "-I$BuildDir/include"
$env:PATH = "$BuildDir/bin;$env:PATH"

# Set PKG_CONFIG_PATH for Meson to find dependencies
$env:PKG_CONFIG_PATH = "$BuildDir/lib/pkgconfig"

$env:MKXPZ_PREFIX = $BuildDir
