# Receitas (VST3) — aprende sound design passo a passo · learn sound design step by step

**PT-PT** e **EN-GB** na mesma interface. Um sintetizador que ensina: escolhes uma **receita** (ex.: lead supersaw), clicas nos **passos** um a um e o plugin
**põe os botões** como nesse passo e explica **o que mudou e porquê**. Os botões que mudaram ficam a laranja. Depois é só mexer neles.
A pt-PT and en-GB synth that teaches: pick a recipe, click the steps one by one and the plugin sets the knobs and explains what changed and why.
Everything is deterministic (hand-curated tables, no randomness, no AI); generic techniques only, no preset copies, no synth or artist names.

## Receitas incluídas · Included recipes
Lead supersaw · Pluck de future bass · Pad quente · Baixo reese · Sub limpo · Baixo wobble (LFO sincronizado com o tempo).
Cada uma tem 5 passos curtos. Each has 5 short steps.

## O sintetizador · The synth
- Duas ondas (serra, quadrada, triangular, seno) com **unison até 7 vozes**, desafinação e largura stereo, sub e ruído.
- Filtro passa-baixo com ressonância, **envolvente do filtro** e **LFO sincronizado com o tempo** (1/2 a 1/32 de nota).
- Envolvente de volume (ataque, queda, sustain, release) e saturação.
- **▶ Ouvir o som** toca notas de teste adequadas a cada receita (acorde, baixo...) em loop, sem precisares de teclado; podes também tocar com o teu teclado MIDI.
  Listen plays suitable test notes for each recipe in a loop; you can also play it with a MIDI keyboard.

## Usar no FL Studio · Using it in FL Studio
1. Copia `Receitas.vst3` para `C:\Program Files\Common Files\VST3\` e faz *Find installed plugins*. Copy it there and rescan.
2. Adiciona o Receitas como gerador, escolhe a receita e carrega em **▶ Ouvir o som**. Add it as a generator, pick a recipe and press Listen.
3. Quando gostares do som, toca-o com o teu piano roll como qualquer outro instrumento. Then play it from the piano roll like any other instrument.

## Compilar · Build
- **GitHub Actions**: push da pasta → artefacto `Receitas-VST3`.
- **Windows (VS 2022 + CMake + Git)**: `cmake -B build -G "Visual Studio 17 2022" -A x64` e `cmake --build build --config Release --target Receitas_VST3`.
- **Só motor + testes**: `cmake -B build -DRECEITAS_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`.
- **Teste de áudio offline** (opcional): `-DRECEITAS_BUILD_RENDER_TEST=ON` → `ReceitasRenderTest`.

## Estado e limites · Status and limits
- Motor testado (1,1 mil verificações: todos os parâmetros das receitas existem e estão nos intervalos, textos em PT e EN, cada passo muda mesmo o som). Compila em Linux e o teste de áudio offline passa em todos os 30 passos (sem NaN, com som, sem clipping).
- **Não foi ouvido nem testado dentro do FL Studio.** O carácter dos sons é uma aproximação e vai precisar de afinação com os teus ouvidos. Not tested inside FL Studio; sound character is approximate.
- Os níveis de volume diferem entre receitas (baixos mais baixos que leads); ajusta no mixer. Levels differ between recipes; adjust in the mixer.
- Os passos aplicam todos os botões de uma vez (se tiveres mexido neles, o passo repõe-nos). Clicking a step sets all knobs, overriding your tweaks.
- JUCE é AGPLv3 ou comercial. O nome "Receitas" é provisório. JUCE is AGPLv3 or commercial; the name is provisional.
