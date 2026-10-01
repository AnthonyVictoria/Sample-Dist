# Sample Distort

Plugin VST3 de distorsión para Mac. Arrastras un audio y ese sample se convierte en el patrón de distorsión. Pensado para FL Studio.

No es un binario: hay que compilarlo en tu Mac (un VST3 de macOS no se puede generar desde Linux).

## Qué hace

Tres modos, todos salen del sample que sueltes:

- **Curva.** El sample se resamplea a una tabla de 4096 puntos y esa tabla es la función de transferencia. Cada muestra de entrada se mapea a un punto de esa curva. Simetría mezcla la curva cruda con una versión impar (menos DC, armónicos impares).
- **Waveset.** Cada semiciclo de la señal se sustituye por el sample, estirado a la duración de ese semiciclo y escalado por el pico. El timbre del sample reemplaza la forma de onda.
- **Patrón.** El sample hace loop a la velocidad de Rate y modula un wavefold. Sirve para distorsión rítmica.

También: drive, mix dry/wet, output, oversampling 4x y filtro de DC.

Formatos de sample: WAV, AIFF, FLAC, MP3, OGG. Se normaliza y se recorta a unos 4 segundos.

## Compilar en Mac

Necesitas Xcode (o las command line tools) y CMake.

```bash
xcode-select --install
brew install cmake
cd SampleDistort
chmod +x build_mac.sh
./build_mac.sh
```

El script clona JUCE, genera un VST3 universal (arm64 + Intel), lo firma en ad-hoc y lo copia a:

```
~/Library/Audio/Plug-Ins/VST3/Sample Distort.vst3
```

También genera un standalone en `build/SampleDistort_artefacts/Release/Standalone/` para probarlo sin DAW.

## FL Studio

1. Reinicia FL Studio.
2. Options > Manage plugins > Find more plugins.
3. Busca "Sample Distort" en la categoría Distortion.
4. Arrastra un audio desde Finder a la ventana del plugin, o usa "Cargar sample".

Si FL no lo ve: Options > File settings y confirma que `~/Library/Audio/Plug-Ins/VST3` está en la lista. Un plugin sin firma de Apple a veces pide permitir en Ajustes del Sistema > Privacidad y seguridad.

## Licencia de JUCE

JUCE es gratuito para uso personal bajo su licencia (AGPL si distribuyes el plugin sin comprar licencia comercial). Si lo vas a vender, hace falta licencia de JUCE.

## Presets

El estado guarda la ruta del sample, no el audio. Si mueves el archivo, vuelve a cargarlo.
