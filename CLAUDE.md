# Receitas — project principles (read before touching the code) / princípios do projeto

VST3 synth (JUCE) that teaches sound design: each "recipe" builds an EDM sound step by step, setting the knobs and explaining why.
Sister of Harmonia, Alvorada, Zenite, Farol and Equilíbrio.

## Non-negotiable rules (set by the author)
1. **No randomness, no AI.** Recipes are hand-curated tables of parameter values; the engine is plain DSP (fixed-seed LCG only for noise).
2. **Only things that work**: every recipe step stays inside sane parameter ranges and the sound is tested offline (finite, audible, no clipping).
3. **No copyright issues**: generic sound-design techniques only; no preset copies, no artist or commercial product/synth names.
4. **Bilingual**: every user-visible string exists in pt-PT and en-GB (`L(lang, pt, en)`). Tests check both.
5. `core/` is pure C++17 with tests (recipes reference valid parameters, values in range); `plugin/` only orchestrates (JUCE).
6. The parameter list lives in `core/Receitas` (`params()`); the plugin builds its parameters and UI from it.

## Structure
- `core/Receitas`          parameter table, recipes and steps (bilingual), state-after-step
- `plugin/SynthEngine.h`   unison oscillators, sub, noise, SVF filter with envelope and tempo-synced LFO, drive
- `plugin/PluginProcessor` polyphonic synth + "listen" loop with the recipe's test notes
- `plugin/PluginEditor`    recipe picker, step list with explanations, auto-generated knobs
