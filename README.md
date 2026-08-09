# Latham Audio Veloria

**Fifty years of synthesis that never happened.**

Veloria is a playable polyphonic Dynamic Stochastic Synthesis instrument: an alternate history of what stochastic synthesis might have become if it had received the same decades of instrument design and musical refinement as analogue, FM and wavetable synthesis.

The waveform itself is the stochastic system. Veloria is not a subtractive synth with random modulation attached.

## Locked architecture

### Four performance field controls

- **AMP WALK** — how readily breakpoint amplitudes move.
- **TIME WALK** — how readily breakpoint durations move.
- **AMP BARRIER** — the amplitude space available to the reflecting walk.
- **TIME BARRIER** — the duration space available to the reflecting walk.

### Eight shaping controls + one order switch

- **AMP DIST / TIME DIST** — Adaptive, Uniform, Gaussian, Logistic, Cauchy and Arcsine mutation statistics.
- **AMP STEP / TIME STEP** — mutation reach.
- **CHAOS** — a true DSS performance macro that increases stochastic reach and relaxes pitch anchoring; it does not add an unrelated noise or distortion stage.
- **POINTS** — 4–12 active waveform breakpoints.
- **PITCH LOCK** — how strongly duration changes are normalised back toward keyboard pitch.
- **CURVE** — interpolation between evolving breakpoints.
- **ORDER 1 / 2** — first-order position walks or second-order step/momentum walks.

ADSR, velocity, mono/polyphony and level are performance architecture around the stochastic generator. Polyphonic/channel aftertouch is intended as expressive stochastic pressure: held notes can be pushed deeper into walk/step energy while slightly relaxing pitch lock.

## Instrument families

Factory families are **GENDYN Core, Piano, Pad, Strings, Bass, Lead, Bell, Pluck, FX and Drums**.

Piano, strings, bells and the other familiar names do not claim acoustic modelling. They are deliberately imagined in the spirit of a 1970s synthesizer attempting those identities through its own stochastic synthesis language.

**DISCOVER should remain inside the selected family grammar**: Piano stays struck and pitched, Pads slow and evolving, Strings bowed/stable, Bass compact, Lead pitch-centred but more aggressive, Bell sparse/heavy-tailed, Pluck short/defined, FX unstable, and Drum discovery percussion-only.

The Drum family uses dedicated stochastic one-shot envelopes rather than the tonal ADSR: kick contraction, a two-engine snare body/wire structure, closed/open hats, crash and tom behaviour.

## Living Planet

The central globe is a live mathematical view, not a screensaver. Its twelve possible nodes correspond to oscillator breakpoint state. POINTS determines how many are active. Barriers should affect spatial spread, step size event energy, distributions/order/curve trajectory character, pitch lock temporal coherence, and real voice energy overall intensity. The five larger visual anchors organise the field but are not fake DSP breakpoints.

## Field identity and recall

Seeded randomness makes patches recallable. **NEW FIELD** changes stochastic identity without changing the patch architecture. User presets retain parameters, drum state and MIDI mappings.

## WHAT IF?

Veloria includes an alternate-history feature tracing an imagined commercial lineage from the 1970s Xenakis System 55 through the Gendy-D Lead, Stochastic-303 and Stochastic-909 to **Veloria 2026 — the instrument from the synthesis history that never happened.** The story explains the same architecture used by the synth; it is not a second or fake synthesis engine.

## Product principles

- **Musical before mathematical** — probability systems become playable controls.
- **Stable identity, living performance** — presets remain recognisable without repeating mechanically.
- **Deterministic when required** — seeded randomness makes patches and sessions recallable.
- **Every visible control is meaningful** — no dummy DSP controls.
- **One coherent synthesis language** — stochastic waveform generation remains the source of the sound.
- **No runtime AI dependency** — conventional C++/JUCE DSP.

## Development

Veloria uses C++20, JUCE and CMake. VST3 is the active test target; Audio Unit and Standalone remain planned targets.

```bash
git clone https://github.com/simonlatham155-tech/veloria.git
cd veloria
cmake -S . -B build
cmake --build build --config Release
```
