# Rubato Bake Feature Brief

**Product Lock:** Composer / Rob 2026-09-13

## Overview
Bake v1 adds in-plugin control to export the *processed* MIDI that Rubato emits as a Standard MIDI File (`.mid`) for offline bounces. Rob can then drop this file on a new track in Logic Pro.

## Behavior

1. **Capture Mode**
   - Arm capture button to enable MIDI recording into bake buffer
   - During playback, accumulate transformed note on/off and non-consumed CCs that Rubato outputs
   - Clear design: Arm capture → play → Bake writes file

2. **Export**
   - Bake button (near XL Mode) writes `.mid` file using JUCE MidiFile
   - Save panel or sensible default name: `Rubato-bake-YYYYMMDD-HHMM.mid`

3. **MIDI Data**
   - Preserve MIDI channels
   - XL-mapped CCs (21–26, 76–77) are parameter automation — don't re-emit those as MIDI in the file
   - Pass through other CCs if present

4. **Tempo**
   - Use tempo from host playhead when available
   - Store in MIDI file tempo track

## Constraints

- AU cannot create Logic Arrange regions — export file only
- Prefer capture-during-play for v1 (Logic doesn't hand the region to the AU for true offline re-render)
- Keep XL CC fix behavior intact (based on PR #3: cursor/fix-xl-cc-handling-8389)
- **Do not change the XL CC map**

## Done When

- ✅ UI has Bake button and capture arm control
- ✅ Playing notes through Rubato then Bake produces a `.mid` that reflects timing/velocity transforms
- ✅ PR opened with short usage note
- ✅ No change to CC 21–26 / 76–77 map
