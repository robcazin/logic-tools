# Unfolding — one-page brief

**Working title:** Unfolding  
**Family:** Companion to Rubato (Logic Tools / Rcaz)  
**Form:** Audio AU (spatial / delay), performance-first  
**Status:** Concept lock — 2026-09-06/07 · not built yet

---

## One line

A jazz musician for DSP: listens to the player, comps and embellishes in complementary ways, lays out when needed — never steamrolls the message.

## Why it exists

Sterile digital delay copies taps. Alive delay (Reaktor modulated feedback, BigTime’s in-loop character) *lives in the recirculation*. Unfolding is that aliveness for organic spatial / environmental work — without requiring a $1k pedal or a mouse mid-phrase.

## Metaphor (Bedrij)

Orest Bedrij’s singularity **‘1’** and **One-and-the-Many**: unity and multiplicity as two views of the same thing.

- The **delay loop is the One**
- **Fluctuation unfolds it into the Many** (time scatter, diffusion, stereo bloom, feedback character)
- Then it can **fold back** toward unity

Modulation is not an LFO painted on a parameter. It is the One↔Many transition *inside* the feedback.

**Zero = One:** ship the first version with **no physical controls**. Earn every knob later when the music demands a constraint.

## Design laws

1. **Respect the input.** Seize the explicit or inherent message; embellish only in complementary ways. Never invent a story that contradicts what was played.
2. **Player owns harmony.** No chord-guessing that overrides tonal intent. Prefer real MIDI when present; audio pitch-to-MIDI is a soft monophonic hint at best.
3. **Stay on the instrument.** Global / shape changes come from performance (levels, density, silence), not from leaving to tweak. Controls arrive only when required.
4. **Patience over reflex.** Never react to the first sample. Analyze across a window, with hysteresis / debounce.

## Core architecture

### Witness
Rolling **dry-input buffer** — the singularity / memory of what the player offered — kept while the wet path grows complex.

Used to:
- Re-anchor or reseed the loop when wet drifts too far
- Compare dry vs wet drift
- Source pitch / gesture analysis from clean signal, not the warped tail

Optional later: **Witness Length** (Short / Medium / Long or bars) as *idea scale* — how patient the analyzer is. Soft / revisit; not dogma.

### Listeners (v0 spine)
- Level / envelope arcs
- Transients (gate unfolds; honor attacks and silence)
- Silence / phrase gaps (invisible “thought end”)
- Optional later: brightness / density — not harmony invention

### Embellishments (inside the loop)
- Feedback character (compress / saturate / soften — BigTime-adjacent, digital)
- Time fluctuation / scatter (unfold)
- Diffusion / ambience smear
- Fold-back toward unity when the idea closes

### Foot / MIDI “end of thought”
**Optional power only.** Default is Auto Witness. A conscious “encapsulate” button fails the jazz test — park until it earns its keep.

## Build order

1. **Zero UI** — Witness + transient/level/silence-driven unfold/fold; audible musical behavior
2. Prove the jazz feel in Logic on real playing
3. Add constraints (Witness Length, Mix, Feedback ceiling, etc.) only when stuck without them
4. XL / foot only after the invisible path works

## Explicitly not

- A BigTime clone or motorized-fader fetish
- A sterile tempo-sync multi-tap with chorus after
- An auto-harmonizer or chart bot
- A panel-first plugin that demands constant twiddling

## Relation to Rubato

| | Rubato | Unfolding |
|---|---|---|
| Domain | MIDI timing / velocity | Audio space / delay |
| Respect | Written velocities, phrase clock | Witness dry message |
| Steer | Set-and-forget then few knobs / XL | Performance-first; Zero=One |
| Persona | Instrument-like velocity/time shaper | Jazz musician for DSP |

Same studio, same ethics — different medium.

---

*Composer · Logic Tools · park beside Rubato until build starts.*
