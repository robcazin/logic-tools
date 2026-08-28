/*
  Rubato Lag - phrase-relative humanize for Logic Pro's Scripter MIDI FX
  ------------------------------------------------------------------------
  Version: 1.6.8 - 2026-08-28

  Every saved Patch (.pst) and every project freezes its OWN private copy of
  this script text - there is no external reference back to this source
  file, and no way to tell two copies apart except by reading them. Bump
  this version line (and the matching "rbt Version" display parameter
  below) every time this file changes, BEFORE re-pasting into Scripter and
  re-saving a Patch. That's the only way to know, just by looking at the
  plugin window, whether a given Patch or project is running current code.
  Built for stylizing sterile/quantized MIDI (e.g. straight-off-the-page
  score conversions) non-destructively. Scripter never modifies the
  underlying region - everything here happens live in the signal path.
  The original track stays untouched; Merge-record a take when you want
  to bake a particular pass into real MIDI, and keep as many stylized
  takes as you want alongside the pristine source.

  What it does
    Timing: delays notes according to a curve across a repeating phrase
      window, so the middle of each phrase lags behind the beat and eases
      back to "on time" by the phrase's end. NoteOff is delayed by the
      same amount as its NoteOn, so note lengths are preserved - only the
      onset moves.
    Velocity: independently shapes loudness across the same phrase
      window - swell louder through the middle, or taper softer, your
      choice, with its own amount and curve shape.

  Trigger Mode - two ways to define "the phrase"
    Cycle Region - active only while Logic's Cycle (loop) mode is on. The
      phrase is exactly whatever you've set the left/right locators to,
      any length, any alignment. Best for shaping one specific, irregular
      passage precisely - set locators, audition, Merge-record, done.
    Fixed Bar Length - active all the time, no Cycle needed. The phrase
      window is "rbt Phrase Length (bars)" bars long and repeats
      continuously from an anchor point. This is the persistent option:
      just play or record normally and every phrase-length chunk gets
      shaped. The anchor starts at bar 1 by default; to reset it to
      Logic's real current position, toggle "rbt Re-Anchor Trigger" - a
      normal automatable checkbox, right alongside every other parameter
      here (all prefixed "rbt " so they're easy to pick out - or search
      for by typing "rbt" - in Logic's automation parameter popup, which
      lists every plugin's params together). Every time its value actually
      CHANGES (off->on or on->off), the script captures Logic's current
      position as the new anchor. That means each re-anchor point needs to
      alternate - on, off, on, off - rather than repeating the same value;
      that's just how automation represents a discrete event on a
      continuous lane, in Logic or any DAW. Option+drag in the automation
      lane gives you clean, snapped-to-value toggles if you want a tidy,
      repeatable shape. This is a live position capture, not a calculated
      one, so it stays
      correct through irregular bar lengths earlier in the piece - see the
      meter-change note below for why that distinction matters. If your
      phrasing is genuinely irregular rather than uniform-length, Cycle
      Region will still track it more faithfully.

  How to use it
    1. Insert Scripter as a MIDI FX on the track, open the script editor,
       paste this in, and Run.
    2. Pick a Trigger Mode. For Cycle Region, set the Cycle locators to
       the phrase first. For Fixed Bar Length, set Phrase Length to match
       your song's structure.
    3. Dial in Amount and Shape for timing, and Velocity Amount and
       Velocity Shape for dynamics, then hit Play to audition by ear.
       (Note: the "rbt " prefix shows on every knob label inside the
       Scripter window too, not just in Logic's automation menu - same
       name field drives both. Worth knowing since it means the in-plugin
       labels get slightly longer as a side effect of the fix.)
    4. To print it permanently: enable recording (Merge, so you keep
       multiple takes to compare) and record through the passage. In
       Fixed Bar Length mode this works over an entire take, not just one
       loop pass.
    5. To save this as one of your "various presets": once the parameters
       are set the way you like, use Scripter's Save As to store it as a
       named Patch (e.g. "Subtle Bell 20ms + Swell", "Late Drag Heavy").
       Each preset is just this same script with different defaults.

  Amount Source (timing only)
    Automation - "rbt Amount (ms)" behaves like any plugin parameter.
      Select the Scripter instance in the track's Automation menu and draw
      a curve; the script reads the live automated value on every note.
    Mod Wheel / CC - "rbt Amount (ms)" instead becomes a ceiling, scaled
      live by the chosen CC (default CC1, the mod wheel; set "rbt CC
      Number" to 11 for an expression pedal). The CC still passes through untouched, so
      the instrument keeps responding to it normally. Move the controller
      at least once before playing; its starting value is treated as 0
      (no lag) until you do.

  Known limits, worth knowing before you rely on this
    - Scripter can only delay a note, never move it earlier - that's why
      the timing curve is "how much later than nominal," not a true
      forward/back rubato. The lag-then-resolve shape is what that
      constraint produces.
    - Position within the phrase is read from the start of the current
      processing block (GetTimingInfo().blockStartBeat), not the exact
      sample of the note-on. Accurate to a few milliseconds - fine for
      phrasing, not for sample-accurate editing.
    - A note near the very end of a phrase can spill into the next phrase
      if pushed past the boundary. The curve returns to 0 near the edges
      specifically to keep this rare, but worth an ear-check.
    - Fixed Bar Length mode reads the CURRENT time signature live on each
      note, which is only reliable within a single constant-meter stretch.
      A phrase window that itself straddles a meter change (e.g. an
      8-bar phrase where one bar switches to 2/4) will compute an
      inconsistent beats-per-bar across notes in that one phrase, so its
      curve will be somewhat distorted. That's a one-phrase, self-limited
      glitch as long as you Re-Anchor at (or near) a phrase boundary
      after the change - it does not drift permanently, because the
      anchor is a captured live position, not a reconstructed one.
    - Velocity shaping is a straight additive offset, clamped to 1-127.
      Large amounts on notes already near the top or bottom of the range
      will compress against that ceiling/floor rather than keep scaling.
*/

var NeedsTimingInfo = true;

var SCRIPT_VERSION = "1.6.8 - 2026-08-28";   // keep in sync with the header comment and the "rbt Version" display param

// Per-note Trace is off unless "rbt Trace Notes" is checked. Trace() is
// cheap-ish, but a busy MIDI track will still flood the console.
function tracing() {
    return GetParameter("rbt Trace Notes") > 0;
}

function noteTag(event) {
    if (GetParameter("rbt Trace Pitch") > 0) return "pitch=" + event.pitch + " ";
    return "";
}

// Meta compressor on MIDI velocity (not the phrase Velocity Amount curve).
// Ratio 1 or velocity at/under threshold leaves the note alone.
function compressVelocity(vel) {
    var thresh = GetParameter("rbt Comp Threshold");
    var ratio = GetParameter("rbt Comp Ratio");
    if (ratio <= 1 || vel <= thresh) return vel;
    var out = Math.round(thresh + (vel - thresh) / ratio);
    if (out < 1) out = 1;
    if (out > 127) out = 127;
    return out;
}

// XL faders in a Custom Mode, CC 21-26 left to right. Continuous knobs only. These CCs are
// eaten here so they do not hit the instrument. Track MIDI Input must
// include the Launch Control XL (or All). Not DAW Mode.
var XL_FADER_CC = [21, 22, 23, 24, 25, 26];
var XL_FADER_PARAM = [
    "rbt Amount (ms)",
    "rbt Velocity Amount",
    "rbt Comp Threshold",
    "rbt Comp Ratio",
    "rbt Phrase Length (bars)",
    "rbt Chord Spread (ms)"
];

function paramIndexByName(name) {
    var i;
    for (i = 0; i < PluginParameters.length; i++) {
        if (PluginParameters[i].name == name) return i;
    }
    return -1;
}

function applyXlFader(cc, val) {
    var slot = -1;
    var i;
    for (i = 0; i < XL_FADER_CC.length; i++) {
        if (XL_FADER_CC[i] == cc) { slot = i; break; }
    }
    if (slot < 0) return false;
    var idx = paramIndexByName(XL_FADER_PARAM[slot]);
    if (idx < 0) return false;
    var p = PluginParameters[idx];
    var scaled;
    if (p.type == "menu") {
        scaled = Math.round((val / 127) * p.maxValue);
        if (scaled < p.minValue) scaled = p.minValue;
        if (scaled > p.maxValue) scaled = p.maxValue;
    } else if (p.type == "checkbox") {
        scaled = (val >= 64) ? 1 : 0;
    } else {
        scaled = p.minValue + (val / 127) * (p.maxValue - p.minValue);
        scaled = Math.round(scaled);
    }
    SetParameter(idx, scaled);
    return true;
}

var activeDelays = {};   // "channel-pitch" -> FIFO queue of ms delays, one per open NoteOn of that pitch.
                          // A queue (not a single value) is required because the same pitch can
                          // retrigger before its previous NoteOff arrives - very common right at
                          // bar starts (a repeated melody note, a bass note reasserting the root).
                          // A single stored value would get overwritten by the second NoteOn and
                          // hand the first note's NoteOff the wrong (usually much smaller) delay,
                          // shrinking its sounding length.
var lastCCValue = 0;     // most recent value (0-127) of the chosen controller; starts as "no lag"
var anchorBeat = 1.0;    // Fixed Bar Length mode's tiling anchor - a captured live position, not calculated
var transportWarm = false;
var warmBlocks = 0;
var paramsLive = false;

// All parameter names are prefixed "rbt " so this script's params are easy to pick
// out - and easy to search for - in Logic's automation parameter popup, which
// otherwise lists every plugin's params together (and RH/LH each have their own
// Scripter instance, so this also disambiguates which track's "rbt Amount" you're
// picking).
var PluginParameters = [
    // Single-choice menu with only one option - Scripter has no plain "label" UI
    // element, so this is the standard trick for a fixed, always-visible readout.
    // It's part of the same script text, so it travels with every copy: whatever
    // this shows in the plugin window IS the version of the code actually running,
    // no console or file comparison needed.
    {name: "rbt Version", type: "menu", valueStrings: [SCRIPT_VERSION], minValue: 0, maxValue: 0, numberOfSteps: 0, defaultValue: 0},

    {name: "rbt Trigger Mode", type: "menu", valueStrings: ["Cycle Region", "Fixed Bar Length"], minValue: 0, maxValue: 1, numberOfSteps: 1, defaultValue: 0},
    {name: "rbt Phrase Length (bars)", type: "lin", minValue: 1, maxValue: 32, numberOfSteps: 31, defaultValue: 4},
    {name: "rbt Phrase Length Mode", type: "menu", valueStrings: ["Manual", "Auto"], minValue: 0, maxValue: 1, numberOfSteps: 1, defaultValue: 0},
    {name: "rbt Re-Anchor Trigger", type: "checkbox", defaultValue: 0},

    {name: "rbt Amount (ms)", type: "lin", minValue: 0, maxValue: 120, numberOfSteps: 120, defaultValue: 40},
    {name: "rbt Shape", type: "menu", valueStrings: ["Symmetric bell", "Early drag", "Late drag"], minValue: 0, maxValue: 2, numberOfSteps: 2, defaultValue: 0},
    {name: "rbt Amount Source", type: "menu", valueStrings: ["Automation", "Mod Wheel / CC"], minValue: 0, maxValue: 1, numberOfSteps: 1, defaultValue: 0},
    {name: "rbt CC Number", type: "lin", minValue: 0, maxValue: 127, numberOfSteps: 127, defaultValue: 1},

    {name: "rbt Velocity Amount", type: "lin", minValue: -40, maxValue: 40, numberOfSteps: 80, defaultValue: 0},
    {name: "rbt Velocity Shape", type: "menu", valueStrings: ["Symmetric bell", "Early swell", "Late swell"], minValue: 0, maxValue: 2, numberOfSteps: 2, defaultValue: 0},
    {name: "rbt Comp Threshold", type: "lin", minValue: 1, maxValue: 127, numberOfSteps: 126, defaultValue: 127},
    {name: "rbt Comp Ratio", type: "lin", minValue: 1, maxValue: 20, numberOfSteps: 19, defaultValue: 1},

    {name: "rbt Chord Spread (ms)", type: "lin", minValue: 0, maxValue: 30, numberOfSteps: 30, defaultValue: 0},

    {name: "rbt Trace Notes", type: "checkbox", defaultValue: 0},
    {name: "rbt Trace Pitch", type: "checkbox", defaultValue: 0},

    {name: "rbt XL Faders", type: "checkbox", defaultValue: 1}
];

// Runs once, immediately, whenever this script is (re)evaluated - on Run,
// and on every project/Patch load, since Scripter re-executes the full
// script text to rebuild its state. Confirms in the Interactive Console
// which version just loaded, independent of the "rbt Version" UI readout.
Trace("Rubato Lag loaded - " + SCRIPT_VERSION);

function curveValue(t, shapeIndex) {
    // t is 0..1, fractional position within the phrase.
    // All three shapes return 0 at t=0 and t=1, so the phrase always
    // resolves back to baseline by its end.
    if (shapeIndex == 0) {
        return Math.sin(Math.PI * t);                    // symmetric hump, peak at the middle
    } else if (shapeIndex == 1) {
        return Math.sin(Math.PI * Math.pow(t, 0.6));      // peak arrives sooner, long gradual release
    } else {
        return Math.sin(Math.PI * Math.pow(t, 1.6));      // stays back longer, resolves late
    }
}

// Returns fractional phrase position (0..1), or null if shaping shouldn't apply right now.
function phraseFraction(info) {
    var triggerMode = GetParameter("rbt Trigger Mode");

    if (triggerMode == 0) {
        // Cycle Region mode
        if (!info.cycling) {
            if (tracing()) Trace("passthrough - Cycle is OFF (info.cycling = false)");
            return null;
        }
        var cycleLen = info.rightCycleBeat - info.leftCycleBeat;
        if (cycleLen <= 0) {
            if (tracing()) Trace("passthrough - cycle length is 0 (left/right cycle beat not set)");
            return null;
        }
        var t = (info.blockStartBeat - info.leftCycleBeat) / cycleLen;
        return t - Math.floor(t);
    } else {
        // Fixed Bar Length mode - tiles continuously from anchorBeat, no Cycle required.
        // anchorBeat defaults to bar 1 and is only ever updated by a live position
        // capture (see ParameterChanged below), never reconstructed by multiplication -
        // that's what keeps it correct across earlier meter changes.
        var beatsPerBar = info.meterNumerator * 4 / info.meterDenominator;
        var lengthMode = GetParameter("rbt Phrase Length Mode");
        var phraseBars = (lengthMode == 1) ? info.meterNumerator : GetParameter("rbt Phrase Length (bars)");
        var phraseLengthBeats = phraseBars * beatsPerBar;

        var relativeBeat = info.blockStartBeat - anchorBeat;
        if (relativeBeat < 0) {
            if (tracing()) Trace("passthrough - before the current anchor (beat " + anchorBeat.toFixed(2) + ")");
            return null;
        }
        var phraseIndex = Math.floor(relativeBeat / phraseLengthBeats);
        var phraseStartBeat = phraseIndex * phraseLengthBeats;
        return (relativeBeat - phraseStartBeat) / phraseLengthBeats;
    }
}

function HandleMIDI(event) {
    var info = GetTimingInfo();

    if (event instanceof ControlChange) {
        if (GetParameter("rbt XL Faders") > 0 && applyXlFader(event.number, event.value)) {
            return;
        }
        if (event.number == GetParameter("rbt CC Number")) {
            lastCCValue = event.value;
        }
        event.send();
        return;
    }

    if (!(event instanceof NoteOn || event instanceof NoteOff)) {
        event.send();
        return;
    }

    var key = event.channel + "-" + event.pitch;

    if (event instanceof NoteOn) {
        var spreadMs = Math.random() * GetParameter("rbt Chord Spread (ms)");
        var t = phraseFraction(info);

        if (t === null) {
            // Phrase-level shaping isn't active here, but chord spread still is -
            // it's a general humanize layer, not tied to a Trigger Mode window.
            event.velocity = compressVelocity(event.velocity);
            var used0 = emitDelayed(event, spreadMs);
            if (!activeDelays[key]) activeDelays[key] = [];
            activeDelays[key].push(used0);
            if (tracing()) Trace("NoteOn " + noteTag(event) + "passthrough spreadMs=" + used0.toFixed(1) + " vel=" + event.velocity);
            return;
        }

        // --- Timing ---
        var ceiling = GetParameter("rbt Amount (ms)");
        var sourceIndex = GetParameter("rbt Amount Source");
        var timingAmount = (sourceIndex == 1) ? ceiling * (lastCCValue / 127) : ceiling;
        var shapeIndex = GetParameter("rbt Shape");
        var delayMs = curveValue(t, shapeIndex) * timingAmount + spreadMs;
        // --- Velocity (set before send so the delayed event carries it) ---
        var velAmount = GetParameter("rbt Velocity Amount");
        var velShapeIndex = GetParameter("rbt Velocity Shape");
        var velDelta = curveValue(t, velShapeIndex) * velAmount;
        var newVelocity = Math.round(event.velocity + velDelta);
        if (newVelocity < 1) newVelocity = 1;
        if (newVelocity > 127) newVelocity = 127;
        event.velocity = compressVelocity(newVelocity);

        var used = emitDelayed(event, delayMs);
        if (!activeDelays[key]) activeDelays[key] = [];
        activeDelays[key].push(used);

        if (tracing()) Trace("NoteOn " + noteTag(event) + "t=" + t.toFixed(2) + " delayMs=" + used.toFixed(1) + " vel=" + event.velocity + " (delta=" + velDelta.toFixed(1) + ")");
    } else {
        // NoteOff: pop the oldest still-open delay for this pitch, so it pairs with
        // whichever NoteOn actually started this particular note, even if the same
        // pitch has retriggered since. If none is queued (e.g. shaping just turned
        // off), there's nothing to reuse, so it passes through untouched.
        var queue = activeDelays[key];
        if (!queue || queue.length === 0) {
            event.send();
            return;
        }
        var delayMs = queue.shift();
        var usedOff = emitDelayed(event, delayMs);
        if (tracing()) Trace("NoteOff " + noteTag(event) + "delayMs=" + usedOff.toFixed(1));
    }
}

// Fires once per actual value change to "Re-Anchor Trigger" (off->on or
// on->off - either direction re-anchors). This is Logic's native automation
// callback, so it only fires on a real transition; drawing the same value
// twice in a row does nothing, which is why each re-anchor point in the
// automation lane needs to alternate rather than repeat.
function Reset() {
    activeDelays = {};
    transportWarm = false;
    warmBlocks = 0;
}

function ProcessMIDI() {
    paramsLive = true;
    var info = GetTimingInfo();
    if (info.playing) {
        warmBlocks += 1;
        if (warmBlocks >= 2) transportWarm = true;
    } else {
        transportWarm = false;
        warmBlocks = 0;
    }
}

function emitDelayed(event, delayMs) {
    var ms = delayMs;
    if (!transportWarm || ms < 1) ms = 1;
    event.sendAfterMilliseconds(ms);
    return ms;
}

function ParameterChanged(param, value) {
    if (!paramsLive) return;
    if (PluginParameters[param].name != "rbt Re-Anchor Trigger") return;
    var info = GetTimingInfo();
    anchorBeat = info.blockStartBeat;
    if (tracing()) Trace("Re-anchored Fixed Bar Length tiling at beat " + anchorBeat.toFixed(2) + " (rbt Re-Anchor Trigger toggled)");
}
