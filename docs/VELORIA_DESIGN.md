# VELORIA — Design, Architecture, and Provenance Record

Copyright © 2026 LATHAMAUDIO. All rights reserved.

This document records the design intent, engineering decisions, product philosophy, and development provenance of the Veloria instrument. It is intended as a living internal record for LATHAMAUDIO.

## 1. Product identity

Veloria is a commercial software instrument built around Dynamic Stochastic Synthesis (DSS). It does not claim to invent DSS. Its design premise is to continue and commercialise a historically significant synthesis lineage by asking what the instrument might have become if stochastic synthesis had received decades of dedicated musical instrument development.

Core positioning:

- stochastic synthesis as the primary sound-generation architecture, not as a modulation novelty
- playable, polyphonic, recallable, performance-oriented design
- a sound that can cover conventional musical roles while retaining a distinct stochastic character
- controlled unpredictability: the sound should feel alive, evolving, and non-looping without becoming unusably chaotic

A concise internal statement of intent is:

> Veloria is not claiming to invent Dynamic Stochastic Synthesis. It is an original instrument architecture built around the idea of continuing and commercialising that lineage.

## 2. Historical grounding

Veloria is historically informed by:

- Iannis Xenakis and Dynamic Stochastic Synthesis / GENDYN
- probability distributions driving breakpoint motion
- amplitude and time random walks
- reflecting barriers
- first- and second-order movement
- continuously evolving waveform geometry

Veloria also includes a Brown IDSS-derived operating model inspired by Andrew R. Brown and Greg Jenkins' Interactive Dynamic Stochastic Synthesizer work from 2004–2005. That mode is treated as an adapted operating model, not an exact recreation.

Brown/Jenkins-inspired ideas incorporated into the alternate model include:

- real-time interaction emphasis
- exponential fine control
- interpolation variants
- zero-crossing anchoring behaviour
- pitch/fixed-segment stabilisation
- stochastic percussion contraction
- historical cascaded second-order behaviour

## 3. Core synthesis architecture

The oscillator is not a conventional analogue-style oscillator with random modulation applied to it.

The waveform itself is generated from a set of amplitude/time breakpoints. Those breakpoint fields evolve stochastically. The sound therefore emerges from the movement of waveform geometry rather than from a static saw, square, sine, or wavetable followed by modulation.

The engine currently includes:

- 12-breakpoint stochastic oscillator field
- variable active breakpoint count
- amplitude walk
- time walk
- amplitude barriers
- time barriers
- selectable probability distributions
- amplitude and time step scaling
- first- and second-order walk behaviour
- pitch stability / pitch-lock behaviour
- interpolation-shape control
- deterministic seeding
- per-voice stochastic fields
- polyphony
- mono mode
- aftertouch pressure influence
- stochastic percussion mode

## 4. Whole-field evolution

A key architectural correction made during development was that the active breakpoint field evolves at waveform-cycle boundaries rather than treating each point as an unrelated continuously updated modulator.

This preserves the conceptual relationship to DSS: the waveform is a coherent stochastic object that evolves from cycle to cycle.

## 5. Second-order behaviour

Historical/Brown second-order motion was corrected to behave as a true cascaded process rather than a superficial smoothing layer.

The intent is that second-order operation exhibits inertia and correlated movement rather than independent first-order randomness.

## 6. Audio-thread safety

An earlier traversal design could allow a segment remainder to grow during stochastic duration changes. This could stall the real-time audio thread in Ableton Live.

The corrected architecture:

- advances using actual cycle-time
- consumes strictly positive time per crossed segment
- uses a bounded maximum of 64 segment crossings per sample
- uses constant-time reflecting barriers
- guards against non-finite values

This safety work was designed to preserve the stochastic architecture rather than replacing it with a simpler conventional oscillator.

## 7. Musical gravity / note-anchored chaos

A major design decision introduced on 2026-08-10 is that CHAOS must remain in service of the played MIDI note or chord.

The previous implementation reduced pitch stability as CHAOS increased. This was musically contrary to the desired instrument behaviour because high CHAOS could pull the stochastic field away from the harmonic identity of the note.

The new principle is:

> The note or chord is the musical attractor. CHAOS may intensify stochastic motion, but it should not destroy the harmonic identity of the entered musical event.

Implementation direction:

- each polyphonic voice remains anchored to its own MIDI note
- in a chord, each voice retains its own pitch centre
- higher CHAOS increases timbral stochastic movement
- higher CHAOS strengthens, rather than weakens, pitch anchoring
- aftertouch may increase expression but should not defeat the harmonic anchor

This is intentionally not framed as an analogue filter replacement. It is a native stochastic-performance principle.

## 8. Control philosophy

Veloria should feel like a musical instrument rather than a laboratory interface.

The guiding rules for controls are:

1. Parameters should be smooth unless they represent genuinely discrete modes.
2. Performance controls should not unexpectedly destroy harmonic intent.
3. Randomness should be bounded, purposeful, and musically legible.
4. The interface should expose a native stochastic language rather than copying analogue terminology unnecessarily.
5. Controls should shape behaviour: movement, probability, density, stability, curvature, barriers, memory, and field structure.

## 9. Current core field controls

The primary stochastic field controls are:

### AMP WALK
Controls the amount of stochastic movement in breakpoint amplitudes.

### TIME WALK
Controls the amount of stochastic movement in breakpoint durations.

### AMP BARRIER
Sets the amplitude reflecting boundary within which amplitude movement is constrained.

### TIME BARRIER
Sets the time/duration reflecting boundary and influences the available temporal field.

## 10. Current eight rotary controls

### AMP DIST
Selects/interprets the probability distribution used for amplitude movement.

### TIME DIST
Selects/interprets the probability distribution used for time movement.

Current distributions include adaptive, uniform, Gaussian, logistic, Cauchy, and arcsine behaviour.

These controls are currently discrete by nature and are therefore a known usability area: a future refinement should consider either explicitly presenting them as mode selectors or providing musically meaningful interpolation/morphing between statistical behaviours rather than making them feel like accidentally stepped continuous knobs.

### AMP STEP
Scales the size of stochastic amplitude steps.

### TIME STEP
Scales the size of stochastic timing steps.

### CHAOS
A macro that intensifies stochastic activity. Its updated design principle is note/chord-aware musical chaos: more stochastic motion, stronger harmonic anchoring.

### POINTS
Controls the number of active stochastic breakpoints. This is inherently discrete because the oscillator field has an integer number of points.

### PITCH LOCK
Controls pitch stability and the degree to which duration geometry is stabilised around the target pitch.

### CURVE
Controls the interpolation shape between breakpoint amplitudes. In Veloria mode it moves from a more direct/linear path toward a smoother curved interpolation. In Brown IDSS mode it morphs through linear, cosine, and square-like interpolation behaviour.

Its audible role is the shape and texture of the transition between stochastic points rather than a conventional filter function.

## 11. Smoothness audit rule

A dedicated control audit is required before final commercial release:

- AMP DIST and TIME DIST should be treated consciously as discrete statistical choices or redesigned as morph controls
- POINTS is correctly discrete because breakpoint count is an integer
- ORDER 1/2 is correctly discrete
- all genuinely continuous controls should sweep without audible parameter stepping introduced by UI quantisation or implementation shortcuts

## 12. Brown IDSS mode

Veloria includes two operating models:

- ENGINE: VELORIA
- ENGINE: BROWN IDSS

The Brown/Jenkins-inspired model is generally tighter and more immediately controlled. During listening tests, the Veloria/original model has been preferred for wide evolving pads, while Brown can be useful for tighter plucks, leads, basses, sequences, and percussive behaviours.

The long-term design direction is not to replace the original model with Brown. Instead, Brown demonstrates ways in which stochastic synthesis can be disciplined musically while the Veloria model retains a more organic stochastic character.

## 13. MIDI and voice architecture

Veloria accepts standard MIDI note input through JUCE.

Current note handling uses JUCE message helpers:

- `isNoteOn(false)` for key-down
- `isNoteOff(true)` for key-up, including MIDI Note On with velocity 0

A debugging period revealed a controller/host MIDI connection state in which Note Off data stopped arriving. Unplugging and reconnecting the MIDI controller restored correct Note Off traffic. This was confirmed as an upstream connection issue rather than an envelope defect.

Veloria nevertheless received several useful improvements during that investigation.

## 14. Held voices vs release tails

A tonal voice now distinguishes between:

- DSP-active voice state
- physically held key state

The UI voice counter is intended to represent currently held tonal voices rather than release tails.

Therefore:

- 8 held keys = 8 VOICES
- releasing one key = 7 VOICES immediately
- releasing all keys = 0 VOICES immediately
- audio release tails may continue after the UI reaches 0

Drum mode remains different: stochastic drum voices are one-shot events and do not depend on MIDI Note Off.

## 15. Stable envelope behaviour

Veloria uses a deterministic internal ADSR implementation for tonal voice envelopes.

Release tails are allowed to continue after the physical key has been released. Once the envelope reaches idle, the DSP voice is retired and made available for reuse.

This separation between held state and envelope state is deliberate.

## 16. Voice allocation

Veloria currently supports up to eight active voices.

When all voices are occupied, the voice allocator may steal the oldest voice. This became visible during the Note Off debugging episode because stale held notes filled all eight slots and the next chord forced replacement of old voices.

The intended semantics of the VOICES display are therefore held allocated voices, capped by the instrument's polyphony.

## 17. Stochastic percussion

Veloria includes a stochastic one-shot percussion architecture rather than conventional sample playback.

Club Beats demonstrates that stochastic synthesis can operate as a percussion generator as well as a tonal oscillator architecture.

Examples include:

- kick contraction
- dual-engine snare body/wire behaviour
- stochastic hats
- crash
- tom behaviour

Drum voices evolve through their own internal lifetime and do not require MIDI Note Off.

## 18. Discover

DISCOVER is a central product feature.

The philosophy is not simply 'randomise every parameter'. Discover should generate usable musical stochastic states and become a fast way for users to find new sounds without needing to understand every mathematical control.

The long-term preset strategy assumes two complementary entry points:

- anchor presets that show familiar musical roles
- Discover as the magic door into unique stochastic states

A known technical debt item is preserving preset-family context across repeated Discover operations so category-aware discovery does not fall back to a generic family after the first randomisation.

## 19. Preset strategy

The factory bank should prioritise musical usefulness first and stochastic novelty second.

Target categories include:

- pads / atmospheres
- strings
- piano / key-like sounds
- plucks
- basses
- leads
- rhythmic / sequence-oriented sounds
- stochastic drums and club percussion
- experimental Brown IDSS sounds
- signature Veloria sounds

The preferred flagship pad direction is the Veloria/original stochastic model because it produces a more organic evolving character. Brown IDSS can dominate tighter sound categories.

The strongest presets should sound intentional first and stochastic second.

## 20. Visual identity

Branding requirements:

- LATHAMAUDIO remains the manufacturer/brand presentation
- the VELORIA wordmark is the main instrument title
- the custom VELORIA title should be compact, geometric, futuristic, and clean
- no promotional photographs are embedded into the normal app interface

The UI uses a dark-space palette with purple/magenta as the dominant stochastic field colours, gold/orange for energy, and cyan as an accent.

## 21. Living stochastic globe

The central visual is intended to represent a live mathematical field rather than a decorative opaque sphere.

Its target character includes:

- thousands of particles
- stochastic filaments/orbits
- five major macro anchors
- smaller true DSP breakpoint nodes
- bright hubs/storms
- asymmetric organic density
- activity extending beyond the implied limb

The globe is intended as a visual signature of Veloria and a representation of the current stochastic state.

## 22. WHAT IF? feature

The in-app WHAT IF section explains the historical and alternate-history premise behind the instrument.

Intended narrative order:

1. real history
2. Xenakis and the foundational DSS moment
3. Brown/Jenkins and the interactive instrument moment
4. the hinge question: what if the synth industry had continued developing this lineage?
5. imagined commercial stochastic instruments
6. Veloria as the 2026 endpoint of that missing development path

Public-facing history should avoid unnecessary references to competitor trademarks or imitative 303/909-style branding.

The historical names Xenakis, Brown, and Jenkins are used in an attribution/contextual sense and should not imply endorsement.

## 23. Manufacturer and metadata

Current manufacturer presentation is LATHAMAUDIO.

The product is currently versioned as Veloria 1.0.0 in CMake.

JUCE baseline was updated from 8.0.4 to 8.0.14 during the MIDI diagnostic period.

## 24. Build discipline

A previous local script continued after a compile failure and could therefore copy/sign an older stale VST3 bundle.

The required build discipline is:

- use `set -e`
- perform a clean build
- verify that the expected VST3 bundle exists before installation
- only claim build success after seeing `[100%] Built target Veloria_VST3`
- quit Ableton before replacing the VST3 when testing a newly built binary

## 25. Product family direction

The broader LATHAMAUDIO concept is a family of instruments based on synthesis branches that did not receive mainstream commercial development.

Current conceptual family:

- VELORIA — Dynamic Stochastic Synthesis
- VELORIA FX — stochastic processing applied to external audio
- WAVORIA — Dynamic Wave Terrain Synthesis
- WAVORIA FX — wave-terrain-derived processing
- MURZIS — evolved spectral / ANS-inspired drawing synthesis
- MURZIS FX — spectral/drawing processing applied to external audio

These names and concepts are product-development ideas and should be treated as internal until commercially cleared.

## 26. Veloria FX concept

Veloria FX should not be a generic multi-effect with random modulation.

Its purpose is to apply Veloria's stochastic philosophy to external audio.

Potential native processors include:

- stochastic filter/energy field
- stochastic delay field
- stochastic spectral field
- stochastic amplitude/dynamics field

A major concept is a macro that moves a familiar input source from mostly original character toward increasingly stochastic transformation without simply replacing it.

## 27. Commercial originality framing

The commercially safe and accurate framing is:

- DSS itself is historical prior art
- Brown/Jenkins IDSS is historical prior art
- Veloria is an original product architecture and implementation built around continuing that lineage
- originality lies in the combination of musical architecture, control philosophy, interaction design, polyphony, stochastic percussion, Discover, live visualisation, preset design, and product framing

No patent novelty claim should be made without professional review.

## 28. Intellectual-property recordkeeping

This repository and its documentation are intended to preserve dated provenance of LATHAMAUDIO's implementation and design decisions.

Copyright protects original code, documentation, graphics, text, and other authored expression. It does not by itself create ownership over abstract synthesis ideas, mathematical principles, or historical techniques.

LATHAMAUDIO should maintain:

- dated Git commit history
- design rationale
- architecture notes
- original UI and interaction decisions
- release notes
- product naming records
- preset-development history

Trademark and patent questions should be reviewed separately with an appropriate professional before commercial filing or enforcement decisions.

## 29. Current unresolved / future work

Known areas still requiring attention include:

- full control smoothness audit
- clearer treatment of discrete AMP DIST / TIME DIST behaviour
- Discover family persistence
- preset bank development and curation
- possible UI resizing / whole-interface proportional scaling
- persistence/automation of engine mode
- thread-safety improvement for engine-mode switching
- removal/consolidation of any obsolete WHAT IF renderer code or old public-facing branded references before release
- performance review of the high-density stochastic globe

## 30. Development principle going forward

The central design test for every future Veloria feature should be:

> Does this make stochastic synthesis more playable, musical, expressive, and useful without turning it into a conventional analogue synthesizer?

If the answer is yes, it belongs in the lineage Veloria is trying to continue.
