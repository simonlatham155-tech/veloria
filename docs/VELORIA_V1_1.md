# VELORIA v1.1 — Engineering Rebuild

VELORIA v1.1 keeps the instrument's stochastic identity and current musical direction, but raises the implementation and release quality to the standard established by WAVORIA.

## Locked identity — do not replace

- Keep the current dynamic stochastic synthesis engine and the recent engine extensions.
- Keep the central stochastic globe as the live engine view.
- Keep the current Veloria purple/magenta/gold colour identity.
- Keep factory presets and editable user presets.
- Keep Discover and New Field.
- Keep the LATHAMAUDIO family branding and overall Veloria visual language.

## v1.1 engineering target

- Sample-accurate MIDI event handling within the audio block.
- Robust polyphony and deterministic voice stealing without changing Veloria's stochastic oscillator identity.
- True stereo voice field rather than duplicating one mono mix to left and right.
- Sustain pedal, pitch bend, poly aftertouch, channel pressure and mod-wheel expression.
- Per-control MIDI CC learn with project recall and visible assignment state.
- Parameter smoothing where abrupt host automation can click or destabilise the stochastic engine.
- Deterministic seeded stochastic behaviour where repeatability is expected.
- Preset/state persistence hardened so factory presets, user presets, MIDI mappings and engine mode restore reliably.
- Preset storage separated from the processor into a dedicated manager rather than growing further inside the audio processor.
- DSP/UI separation: the globe reads published engine state; the UI never drives synthesis timing.
- Stereo-safe output gain and finite-value protection.
- Dependency-free DSP tests for determinism, bounds, stereo activity, release behaviour and stochastic control safety.
- VST3, AU and Standalone targets, with warnings enabled and a repeatable Release build.

## Explicit non-goals

- Do not turn Veloria into Wavoria.
- Do not replace stochastic waveform generation with conventional oscillators plus random modulation.
- Do not remove Chaos, Boundary, Rate, Jump, Correlation, Breakpoints, Walk Order, Pitch Lock or the current stochastic controls.
- Do not redesign the globe into decorative artwork.
- Do not change Veloria to Wavoria's jade colour scheme.

## Release identity

Target version: **1.1.0**

The result should feel like the same Veloria instrument, but engineered as a finished LATHAMAUDIO product rather than an experimental build.