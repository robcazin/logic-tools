# ccMeter

JUCE Audio Unit MIDI FX plugin that displays and filters a selected MIDI CC value.

## Features

- Large vertical bar meter showing CC value (0–127)
- Selectable CC number (default 119, for Rubato Lag phrase-curve)
- Filters the watched CC from MIDI stream (downstream instruments don't see it)
- Passes all other MIDI (notes, pitchbend, other CCs) unchanged

## AU Codes

- Manufacturer: `Rcaz`
- Plugin: `Ccmt`
- Type: `aumi` (kAudioUnitType_MIDIProcessor)

## Build on macOS

Prerequisites: Xcode, CMake 3.22+

```bash
cd plugins/cc-meter
cmake -B build -G Xcode
cmake --open build
```

In Xcode: Build the `ccMeter_AU` target.

Or build from command line:
```bash
cmake --build build --config Release --target ccMeter_AU
```

The built `.component` will be in `build/ccMeter_artefacts/Release/AU/`.

## Install

```bash
cp -r build/ccMeter_artefacts/Release/AU/ccMeter.component ~/Library/Audio/Plug-Ins/Components/
```

Restart Logic or rescan plugins (Options > Plug-in Manager > Reset & Rescan).

## Validation

`auval -v aumi Ccmt Rcaz` often fails for MIDI FX even when Logic loads successfully. If Logic sees it under MIDI FX, ignore auval failures.

## Version

0.1.0
