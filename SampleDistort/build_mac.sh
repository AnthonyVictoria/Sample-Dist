#!/bin/bash
# Compila Sample Distort como VST3 universal (Apple Silicon + Intel) y lo copia a la carpeta de plugins del usuario.
set -euo pipefail
cd "$(dirname "$0")"

if ! xcode-select -p >/dev/null 2>&1; then
  echo "Instala las herramientas de Xcode: xcode-select --install"
  exit 1
fi

if [ ! -d JUCE/CMakeLists.txt ] && [ ! -f JUCE/CMakeLists.txt ]; then
  echo "Clonando JUCE..."
  git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE
fi

cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release

VST3="build/SampleDistort_artefacts/Release/VST3/Sample Distort.vst3"
if [ ! -d "$VST3" ]; then
  echo "No encontré el VST3 en $VST3"
  find build -name "*.vst3" -maxdepth 6
  exit 1
fi

# Ad-hoc sign para que macOS y FL Studio lo carguen sin certificado de desarrollador.
codesign --force --deep --sign - "$VST3"

DEST="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$DEST"
rm -rf "$DEST/Sample Distort.vst3"
cp -R "$VST3" "$DEST/"
echo ""
echo "Listo: $DEST/Sample Distort.vst3"
echo "En FL Studio: Options > Manage plugins > Find more plugins (o rescan VST3)."
