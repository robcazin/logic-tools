# Logic Tools

## Rubato MIDI FX Plugin

Rubato is a unified Logic Pro MIDI FX plugin that combines phrase-relative MIDI shaping with visual metering. It ports the Scripter 1.6.18 behavior into a native AU plugin.

### Features

**MIDI Processing:**
- **Phrase-relative timing**: Delay notes according to a curve across a repeating phrase window
- **Velocity shaping**: Independently shape loudness across the same phrase window
- **Compression**: Two-sided velocity compressor with threshold and ratio controls
- **Chord spread**: Random timing variation for chord humanization
- **Pulse/Drift**: Add unipolar pulse and 2-bar drift modulation

**Visual Metering:**
- **Phrase curve meter**: 0-127 display of the live phrase velocity curve (atomic, not automatable)
- **16-band keyboard register meters**: Eight octaves from MIDI C0=12, split C-F vs F#-B
- **In/Out toggle**: View written NoteOn velocity vs. processed velocity after Rubato

**Curve Shapes:**
- Symmetric bell: sin(π·t)
- Early drag: sin(π·t^0.6)
- Late drag: sin(π·t^1.6)

**Trigger Modes:**
- **Cycle Region**: Uses host loop locators (requires Logic Cycle mode)
- **Fixed Bar Length**: Tiles continuously from anchor point

**XL Mode:**
- Eats CC 21-26 and maps to Amount, Boost, Comp Threshold, Comp Ratio, Phrase Length, Chord Spread
- Other CCs pass through unchanged

### Building on macOS

#### Prerequisites
- CMake 3.22 or later
- Xcode with Command Line Tools
- JUCE 8.0.15 (fetched automatically via CMake)

#### Build Steps

```bash
cd plugins/cc-meter
mkdir build && cd build
cmake -G Xcode ..
cmake --build . --config Release
```

The AU plugin will be built to:
```
build/Rubato_artefacts/AU/Rubato.component
```

#### Installation

Copy the built component to Logic's plugin folder:
```bash
cp -r build/Rubato_artefacts/AU/Rubato.component ~/Library/Audio/Plug-Ins/Components/
```

Then rescan plugins in Logic Pro or restart the application.

### Using in Logic Pro

1. Insert Rubato as a MIDI FX on any track (can replace Scripter + ccMeter)
2. Choose a Trigger Mode:
   - **Cycle Region**: Set Logic's Cycle locators first
   - **Fixed Bar Length**: Set Phrase Length to match your song structure
3. Adjust Time controls: Amount (ms), Shape, Phrase Length, Chord Spread
4. Adjust Velocity controls: Boost, Replace, Velocity Shape, Comp Threshold, Comp Ratio
5. Add Pulse/Drift for additional modulation
6. Toggle In/Out to compare written vs. processed velocities in the meters

### Technical Details

- **AU Type**: `kAudioUnitType_MIDIProcessor` (aumi)
- **Manufacturer**: Rcaz
- **Plugin Code**: Rbto (changed from ccMeter's Ccmt to allow coexistence)
- **JUCE Version**: 8.0.15
- **Delay Implementation**: Uses timestamped MidiBuffer delay queue (JUCE equivalent of Scripter's `sendAfterMilliseconds`)
- **NoteOff Pairing**: FIFO per channel-pitch to preserve note lengths

### Notes

- Does NOT emit CC 119 (unlike the old ccMeter)
- Phrase meter reads the same curve used for velocity shaping
- Peak-hold decay on keyboard meters for readable chord visualization
- Clipped fills inside meter rectangles to prevent overdraw
- Logic Latch mode will not print the phrase curve value (it's atomic, not an automatable parameter)

### Cycle Mode Investigation

If Logic does not provide loop locators via the host playhead API, Fixed Bar Length mode is sufficient for v1. The Cycle Region mode implementation is present and will work when Logic provides the necessary host information.

---

## Scripter Archive

Live Scripter master is `scripter/rubato-lag.js` (currently Rubato Lag 1.6.8).
Paste that file into Logic Scripter. Version lives inside the file (`SCRIPT_VERSION`).

Older numbered copies are in `scripter/archive/`.
