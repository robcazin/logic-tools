# LCXL monitor / Cue 2 / scrub helper

Mac-side MIDI helper that listens to a Novation Launch Control XL and provides:
- UA Console monitor/cue VCA and mute control
- Absolute-to-relative scrub conversion for Logic playhead navigation
- Rubato CC forwarding to a virtual MIDI port

## Files

- **lcxl-monitor-hotkeys.swift** — Swift source for the helper daemon
- **com.robcazin.lcxl-monitor-hotkeys.plist** — LaunchAgent that runs the helper at login
- **com.robcazin.ni-agents.plist** — LaunchAgent that opens Native Instruments' hardware agents at login (needed for Komplete Kontrol S61 screens and Logic integration)

## Features

### Console (UA Mixer Engine :4710)
- **CC 27** — monitor VCA (fader 7)
- **CC 28** — Cue 2 / sub VCA (fader 8)
- **CC 43** — monitor mute (button 7)
- **CC 44** — Cue 2 mute (button 8); fader 8 clears mute

**Important:** Do NOT MIDI-Learn CC 27 / 28 / 43 / 44 in Logic. The helper drives UA Console directly.

### Scrub bridge (abs → Key Command pulses)
Listens to XL **CC 29 / 30 / 31** (absolute knobs), converts deltas into Key Command pulses on virtual MIDI port **`LCXL Scrub KC`**:
- CC 29 (bar) → CC 90 (forward) / CC 91 (back)
- CC 30 (beat) → CC 92 (forward) / CC 93 (back)
- CC 31 (division) → CC 94 (forward) / CC 95 (back)

Pulses are sent as CC value 127 then 0 on channel 1.

In Logic:
1. Go to **Logic Pro > Control Surfaces > Learn Assignment**
2. For Playhead Forward by Bar: tweak CC 29 clockwise → Logic learns CC 90 from LCXL Scrub KC
3. For Playhead Rewind by Bar: tweak CC 29 counter-clockwise → Logic learns CC 91 from LCXL Scrub KC
4. Repeat for Beat (CC 30) and Division (CC 31)

First knob move only arms (no jump). Bandwidth: a few bytes per encoder tick.

### Rubato CC forwarding
Forwards CC 21-26 / 76 / 77 from the XL to virtual port **`LCXL Rubato`** so the Rubato plugin (or Scripter) can consume them without Logic's Control Surface swallowing them.

## Building

The helper requires macOS frameworks (CoreMIDI, Foundation, Darwin).

```bash
swiftc -O lcxl-monitor-hotkeys.swift -o ~/Library/Application\ Support/logic-tools/lcxl-monitor-hotkeys
```

Create the target directory first if needed:

```bash
mkdir -p ~/Library/Application\ Support/logic-tools
```

## Installing LaunchAgents

### LCXL monitor hotkeys helper

1. Copy the plist to your LaunchAgents folder:

```bash
cp com.robcazin.lcxl-monitor-hotkeys.plist ~/Library/LaunchAgents/
```

2. Load the agent:

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.robcazin.lcxl-monitor-hotkeys.plist
```

3. Check the log to verify it's running:

```bash
tail -f ~/Library/Logs/lcxl-monitor-hotkeys.log
```

To unload:

```bash
launchctl bootout gui/$(id -u) ~/Library/LaunchAgents/com.robcazin.lcxl-monitor-hotkeys.plist
```

### Native Instruments agents

1. Copy the plist to your LaunchAgents folder:

```bash
cp com.robcazin.ni-agents.plist ~/Library/LaunchAgents/
```

2. Load the agent:

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.robcazin.ni-agents.plist
```

3. Check the log:

```bash
tail -f ~/Library/Logs/ni-agents.log
```

To unload:

```bash
launchctl bootout gui/$(id -u) ~/Library/LaunchAgents/com.robcazin.ni-agents.plist
```

## Troubleshooting

- The helper logs to `~/Library/Logs/lcxl-monitor-hotkeys.log`
- Check that the Launch Control XL is connected and visible in Audio MIDI Setup
- The UA Console mixer must be running on localhost:4710
- Virtual MIDI ports are created automatically by the helper on startup
