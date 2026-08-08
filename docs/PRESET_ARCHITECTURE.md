# Veloria Mk-I preset architecture

Veloria's factory bank is an alternate-history instrument experiment: what might a commercial stochastic keyboard of the late 1970s have called PIANO, STRINGS, BRASS/BASS, LEAD, BELL, PLUCK, PAD, DRUMS and FX?

The target is recognition, not acoustic realism. A Piano preset must make a sincere stochastic attempt at the *gesture* of a 1970s synth piano; it must not be renamed merely because the attempt has its own character.

## Global musical contract

- DSS remains the sound source. Do not solve factory sounds by silently substituting conventional VA oscillators, samples or a normal subtractive filter.
- Pitched presets obey the keyboard: note-on creates the voice and note-off always closes it through the final voice envelope.
- Drums are the exception: one-shot stochastic events, with musically appropriate choke behaviour where implemented.
- MIDI pitch remains anchored by cycle-duration normalisation while internal waveform geometry evolves.
- Interesting failed emulations are discoveries, not bugs, but the factory label must still have an audible reason for its name.
- `Discover` explores *within the selected instrument species*. It is not unrestricted randomisation.
- `New Field` changes stochastic history/seed without changing the species grammar.

## Native DSS dimensions

Veloria should evolve inward before adding conventional synth furniture. Priority dimensions are:

1. amplitude and duration random-walk energy;
2. reflecting-barrier freedom;
3. second-order memory/correlation;
4. probability distribution;
5. transient contraction/expansion of stochastic energy;
6. breakpoint complexity/density;
7. related but independently evolving stochastic populations;
8. stable-pitch <-> structural-chaos performance travel.

The oscillator now supports adaptive, Uniform, Gaussian-like, Logistic, Cauchy and Arcsine probability behaviour. Adaptive mode intentionally favours small organic changes at low walk energy and increasingly heavy-tailed behaviour as stochastic energy rises.

## Factory species

### GENDYN Core
Reference voice. Exposes the underlying stochastic language without pretending to be an acoustic family.

### Piano
1970s synth-piano target, not Steinway modelling.

Gesture: immediate bright/irregular strike -> rapid contraction of stochastic energy -> simpler pitched body -> decay. Key release closes the voice. Future refinement may use a small related multi-voice population, but should not turn into sample/physical-model piano replacement.

Discover fence: fast attack, medium decay, low sustain, short/medium release, tightly anchored pitch, low sustain walk, stronger attack walk.

### Pad / Flow
Primary native showcase for DSS.

Gesture: slow birth -> stable chord identity -> continuous non-periodic spectral migration. Duration-breakpoint motion is encouraged because DSS can create phasing/flanging-like spectral travel without a cyclic phaser/LFO. Key release still closes the field.

Discover fence: slow attack/release, high sustain, small-to-moderate walk, broad but safe barriers, smooth probability behaviour. Future stereo form: two related seeds/populations that slowly diverge rather than conventional chorus.

### Strings
1970s string-machine *attempt* through stochastic synthesis.

Gesture: soft bow-like onset, strong pitch identity, microscopic continuous friction/ensemble movement, graceful release. Prefer correlated small movement over obvious modulation cycles.

Discover fence: low walk energy, moderate/high structure, high sustain, smooth distributions.

### Bass
1970s synth-bass target. This is the restraint test.

Gesture: immediate stable fundamental with controlled living harmonics. It must survive a club mix and must not wander out of pitch merely to demonstrate stochasticity.

Discover fence: tight time barriers, low duration walk, low/moderate amplitude movement, short attack/release. Structural chaos range intentionally limited.

### Lead
Playable melodic stochastic voice.

Gesture: strong pitch anchor, more expressive mutation than Bass, enough harmonic motion to feel alive under a held note without dissolving the melody.

Discover fence: tight pitch identity, moderate walk/mirror freedom, short attack and performance-friendly release.

### Bell
1970s electronic bell attempt, not a physical-model church bell.

Gesture: instant irregular metallic strike -> inharmonic stochastic body -> long settling decay. Probability choice should be auditioned rather than assuming one distribution is universally correct.

Discover fence: immediate attack, low sustain, long decay/release, relatively adventurous attack behaviour.

### Pluck
Stochastic struck/plucked gesture.

Gesture: spectrally rich stochastic excitation -> fast contraction -> residual pitched vibration -> silence. This inherits the percussion discovery while remaining a pitched keyboard voice whose note-off can close it.

Discover fence: near-zero attack, short decay, low sustain, short release, controlled pitch.

### FX
Native stochastic wilderness.

Gesture: pitch <-> unstable pitch <-> spectral tearing <-> swarm <-> recovery. This is where wide barriers, heavy tails and large walk energy are allowed.

Discover fence: broadest ranges in the instrument, but output and voice lifetime remain bounded.

### Drums
Playable stochastic kit across familiar keyboard positions: kick, snare body + wire population, closed/open hats, cymbal/crash and toms.

Gesture: impact starts with high stochastic energy and contracts toward stability/silence. Drum Discover remains fenced into percussion grammar. No fallback to ordinary pitched synthesis while Drum mode is active.

## Performance vocabulary

The long-term front-panel vocabulary should describe musical consequences rather than mathematics:

- **STRUCTURE**: constrained/stable <-> structurally open/chaotic. Candidate master performance control analogous in importance, not mechanism, to analogue cutoff.
- **MOTION**: frozen <-> continuously evolving.
- **TENSION**: gentle local probability <-> violent/heavy-tailed mutation.
- **MEMORY**: weakly related change <-> strongly correlated historical evolution.

These are macro controls over DSS variables, not renamed copies of single parameters. Each species constrains their useful range.

A crucial live gesture is recoverability: Veloria should eventually be able to travel from a tuned groove into structural chaos and return decisively to a stable pitched state. Chaos that cannot be musically recovered is an effect; controllable collapse-and-return is an instrument.

## Development order

1. Validate the stochastic Drum kit by ear.
2. Build Pad/Flow as the first deliberate demonstration of intrinsic non-periodic spectral motion.
3. Make Piano a convincing *1970s stochastic synth-piano attempt* using transient stochastic contraction.
4. Give every remaining species its own Discover fence.
5. Expose/audition probability distributions deliberately per species.
6. Investigate variable breakpoint complexity.
7. Investigate related dual/multi-population voices for Pad, Strings, Piano and metallic/percussion families.
8. Only then decide which conventional effects, if any, are actually necessary.
