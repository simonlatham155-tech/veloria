# Latham Audio Veloria

**Fifty years of synthesis that never happened.**

Veloria is an experimental software synthesizer exploring what stochastic synthesis might have become if it had received the same sustained instrument design, engineering, and musical refinement as analogue, FM, wavetable, and granular synthesis.

The goal is not to reproduce Xenakis's GENDYN as an academic exercise. The goal is to create a playable, polyphonic instrument whose sounds have a stable identity while remaining alive, variable, and capable of controlled evolution.

## Product principles

- **Musical before mathematical** — probability systems are translated into controls musicians can understand.
- **Stable identity, living performance** — presets remain recognisable without repeating mechanically.
- **Deterministic when required** — seeded randomness makes patches and sessions recallable.
- **No expensive runtime AI dependency** — the instrument is conventional DSP software.
- **Original instrument design** — not another subtractive synth with random modulation attached.

## First milestone

Build one reliable stochastic oscillator capable of producing ten clearly distinct, musically useful sounds:

- bass
- lead
- pad
- pluck
- bell
- percussion
- drone
- texture
- vocal/formant-like tone
- evolving cinematic sound

See [`docs/roadmap.md`](docs/roadmap.md) and [`docs/engine.md`](docs/engine.md).

## Planned formats

- VST3
- Audio Unit
- Standalone

AAX is a possible later target.

## Development

Veloria uses C++20, JUCE, and CMake.

```bash
git clone https://github.com/simonlatham155-tech/veloria.git
cd veloria
cmake -S . -B build
cmake --build build --config Release
```

JUCE is fetched automatically by CMake during configuration.

## Status

Early architecture and oscillator prototype stage. Interfaces, file formats, and DSP behaviour will change.
