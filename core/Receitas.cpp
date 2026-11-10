#include "Receitas.h"

#include <algorithm>

namespace rec {

std::string L(Lang l, const char* pt, const char* en) { return l == Lang::PT ? pt : en; }

std::string groupName(int g, Lang l) {
    switch (g) {
        case GOsc: return L(l, "OSCILADORES", "OSCILLATORS");
        case GFilter: return L(l, "FILTRO", "FILTER");
        case GAmp: return L(l, "VOLUME (ENVOLVENTE)", "VOLUME (ENVELOPE)");
        case GFx: return L(l, "SOM E ESPAÇO", "TONE AND SPACE");
        case GLfo: return "LFO";
        default: return L(l, "SAÍDA", "OUTPUT");
    }
}

const std::vector<ParamSpec>& params() {
    static const std::vector<ParamSpec> p = {
        {"wave1", "Onda 1", "Wave 1", 0, 3, 1, 4, false, GOsc},
        {"wave2", "Onda 2", "Wave 2", 0, 3, 0, 4, false, GOsc},
        {"semi2", "Afinação 2 (semitons)", "Tuning 2 (semitones)", -24, 24, 0, 0, false, GOsc},
        {"mix2", "Mistura da onda 2", "Wave 2 mix", 0, 1, 0, 0, false, GOsc},
        {"voices", "Vozes (unison)", "Voices (unison)", 1, 7, 1, 0, false, GOsc},
        {"detune", "Desafinação (cents)", "Detune (cents)", 0, 60, 0, 0, false, GOsc},
        {"sub", "Sub (oitava abaixo)", "Sub (octave below)", 0, 1, 0, 0, false, GOsc},
        {"noise", "Ruído", "Noise", 0, 1, 0, 0, false, GOsc},
        {"cutoff", "Corte (Hz)", "Cutoff (Hz)", 40, 18000, 18000, 0, true, GFilter},
        {"res", "Ressonância", "Resonance", 0, 0.9, 0.1, 0, false, GFilter},
        {"envAmt", "Envolvente do filtro", "Filter envelope", -1, 1, 0, 0, false, GFilter},
        {"fDecay", "Queda do filtro (s)", "Filter decay (s)", 0.02, 2, 0.3, 0, true, GFilter},
        {"fSustain", "Sustain do filtro", "Filter sustain", 0, 1, 1, 0, false, GFilter},
        {"aAttack", "Ataque (s)", "Attack (s)", 0.001, 2, 0.005, 0, true, GAmp},
        {"aDecay", "Queda (s)", "Decay (s)", 0.02, 3, 0.3, 0, true, GAmp},
        {"aSustain", "Sustain", "Sustain", 0, 1, 1, 0, false, GAmp},
        {"aRelease", "Release (s)", "Release (s)", 0.01, 4, 0.2, 0, true, GAmp},
        {"drive", "Saturação", "Drive", 0, 1, 0, 0, false, GFx},
        {"width", "Largura stereo", "Stereo width", 0, 1, 0.5, 0, false, GFx},
        {"lfoAmt", "Quantidade do LFO", "LFO amount", 0, 1, 0, 0, false, GLfo},
        {"lfoRate", "Velocidade do LFO (nota)", "LFO rate (note)", 0, 4, 2, 5, false, GLfo},
        {"volume", "Volume", "Volume", 0, 1, 0.7, 0, false, GOut},
    };
    return p;
}

int paramIndex(const std::string& id) {
    const auto& p = params();
    for (size_t i = 0; i < p.size(); ++i) if (id == p[i].id) return (int) i;
    return -1;
}

std::vector<std::string> choiceNames(const std::string& id, Lang l) {
    if (id == "wave1" || id == "wave2") return {L(l, "Serra", "Saw"), L(l, "Quadrada", "Square"), L(l, "Triangular", "Triangle"), L(l, "Seno", "Sine")};
    if (id == "lfoRate") return {"1/2", "1/4", "1/8", "1/16", "1/32"};
    return {};
}
double lfoRateBeats(int c) {
    static const double v[5] = {2.0, 1.0, 0.5, 0.25, 0.125};
    return v[std::max(0, std::min(4, c))];
}

// ------------------------------------------------------------------ receitas
namespace {
struct S { const char *tp, *te, *wp, *we; std::vector<std::pair<std::string, double>> set; };
struct R { const char *np, *ne, *cp, *ce, *up, *ue; std::vector<int> notes; double rate, len; std::vector<S> steps; };

const std::vector<R>& recipes() {
    static const std::vector<R> r = {
        {"Lead supersaw", "Supersaw lead", "Lead", "Lead",
         "O lead largo e brilhante dos drops: várias serras desafinadas a soar como uma só.", "The wide, bright drop lead: several detuned saws sounding as one.",
         {57, 64, 69, 72}, 2.0, 1.8,
         {{"Começa com uma serra", "Start with one saw", "A serra tem todos os harmónicos, por isso é brilhante e rica: é a matéria-prima dos leads.", "A saw contains every harmonic, so it is bright and rich: it is the raw material of leads.",
           {{"wave1", 0}, {"voices", 1}, {"cutoff", 18000}}},
          {"Empilha vozes (unison)", "Stack voices (unison)", "Sete serras ao mesmo tempo soam muito maiores do que uma. Ainda soa 'fino' porque estão todas afinadas igual.", "Seven saws at once sound much bigger than one. It still sounds thin because they are all tuned identically.",
           {{"voices", 7}}},
          {"Desafina de leve", "Detune slightly", "Pequenas diferenças de afinação criam batimentos entre as vozes: é isso que cria a sensação de largura e brilho 'supersaw'.", "Tiny tuning differences create beating between the voices: that is what gives the 'supersaw' width and shimmer.",
           {{"detune", 22}, {"width", 0.85}}},
          {"Doma os agudos com o filtro", "Tame the top with the filter", "O topo cru é áspero. Um filtro passa-baixo a cerca de 7 kHz tira o chiado e deixa o brilho.", "The raw top is harsh. A low-pass filter around 7 kHz removes the fizz and keeps the shine.",
           {{"cutoff", 7000}, {"res", 0.15}}},
          {"Define o volume ao longo do tempo", "Shape the volume over time", "Ataque curto para o lead entrar a tempo, sustain alto para as notas aguentarem e um release curto para não encavalitar notas.", "A short attack so the lead enters on time, a high sustain so notes hold and a short release so notes do not pile up.",
           {{"aAttack", 0.01}, {"aDecay", 0.4}, {"aSustain", 0.85}, {"aRelease", 0.35}}}}},

        {"Pluck de future bass", "Future bass pluck", "Pluck", "Pluck",
         "O acorde curto e brilhante do future bass: abre e fecha depressa, como uma corda beliscada.", "The short, bright future bass chord: it opens and closes fast, like a plucked string.",
         {57, 60, 64, 67}, 0.5, 0.4,
         {{"Duas ondas em camada", "Layer two waves", "Uma serra e uma quadrada misturadas dão corpo (a quadrada tem só harmónicos ímpares, mais 'oca').", "A saw and a square mixed give body (a square has only odd harmonics, a more 'hollow' sound).",
           {{"wave1", 0}, {"wave2", 1}, {"mix2", 0.5}, {"cutoff", 18000}}},
          {"Unison e largura", "Unison and width", "Cinco vozes ligeiramente desafinadas tornam o acorde largo, típico do estilo.", "Five slightly detuned voices make the chord wide, typical of the style.",
           {{"voices", 5}, {"detune", 18}, {"width", 0.8}}},
          {"Filtro que abre e fecha", "A filter that opens and closes", "O segredo do pluck: o filtro começa aberto e fecha rapidamente. A envolvente do filtro faz esse 'pop' de ataque brilhante.", "The secret of a pluck: the filter starts open and closes quickly. The filter envelope makes that bright attack 'pop'.",
           {{"cutoff", 1200}, {"res", 0.2}, {"envAmt", 0.75}, {"fDecay", 0.25}, {"fSustain", 0}}},
          {"Volume curto", "Short volume", "Sustain a zero faz o som morrer sozinho, como uma corda: a nota termina mesmo que continues a carregar na tecla.", "Zero sustain makes the sound die away by itself, like a string: the note ends even if you hold the key.",
           {{"aAttack", 0.002}, {"aDecay", 0.35}, {"aSustain", 0}, {"aRelease", 0.2}}},
          {"Um toque de saturação", "A touch of drive", "Saturação leve acrescenta harmónicos e faz o pluck 'morder' na mistura.", "Light drive adds harmonics and makes the pluck 'bite' in the mix.",
           {{"drive", 0.18}}}}},

        {"Pad quente", "Warm pad", "Pad", "Pad",
         "A cama de acordes que enche o espaço atrás da melodia: lenta, larga e suave.", "The chord bed that fills the space behind the melody: slow, wide and soft.",
         {57, 60, 64, 69}, 4.0, 3.8,
         {{"Duas ondas suaves", "Two soft waves", "Serra mais triangular uma oitava acima: corpo com um brilho suave por cima.", "A saw plus a triangle an octave up: body with a soft shine on top.",
           {{"wave1", 0}, {"wave2", 2}, {"semi2", 12}, {"mix2", 0.5}}},
          {"Mais vozes, mais desafinação", "More voices, more detune", "Um pad precisa de movimento lento: vozes desafinadas criam um 'coro' natural.", "A pad needs slow movement: detuned voices create a natural 'chorus'.",
           {{"voices", 5}, {"detune", 14}}},
          {"Filtro suave", "Soft filter", "Cortar os agudos faz o pad ficar atrás da melodia em vez de competir com ela.", "Rolling off the highs keeps the pad behind the melody instead of competing with it.",
           {{"cutoff", 2200}, {"res", 0.1}}},
          {"Ataque e release lentos", "Slow attack and release", "Um ataque lento faz o acorde crescer, e um release longo une um acorde ao seguinte sem cortes.", "A slow attack makes the chord swell, and a long release joins one chord to the next without gaps.",
           {{"aAttack", 0.6}, {"aDecay", 1.0}, {"aSustain", 0.9}, {"aRelease", 1.5}}},
          {"Largura e movimento lento", "Width and slow movement", "Máxima largura stereo e um LFO muito lento no filtro dão vida ao pad sem chamar a atenção.", "Maximum stereo width and a very slow LFO on the filter bring the pad to life without drawing attention.",
           {{"width", 0.9}, {"lfoAmt", 0.15}, {"lfoRate", 0}}}}},

        {"Baixo reese", "Reese bass", "Baixo", "Bass",
         "O baixo 'rosnado' e movimentado: duas serras ligeiramente desafinadas que batem uma contra a outra.", "The moving, growling bass: two slightly detuned saws beating against each other.",
         {33}, 2.0, 1.9,
         {{"Duas serras", "Two saws", "Duas serras iguais são a base do reese: muita energia e harmónicos.", "Two identical saws are the base of the reese: lots of energy and harmonics.",
           {{"wave1", 0}, {"wave2", 0}, {"mix2", 0.5}, {"cutoff", 18000}}},
          {"Desafina uma contra a outra", "Detune one against the other", "A pequena diferença de afinação cria um batimento lento: é o 'rosnado' que se mexe sozinho.", "The small tuning difference creates a slow beating: that is the 'growl' that moves by itself.",
           {{"voices", 2}, {"detune", 12}}},
          {"Filtro para escurecer", "Filter to darken", "Um baixo precisa de menos agudos para não competir com a melodia. A ressonância dá carácter.", "A bass needs fewer highs so it does not compete with the melody. Resonance adds character.",
           {{"cutoff", 450}, {"res", 0.25}}},
          {"Sub para o peso", "Sub for weight", "Um seno uma oitava abaixo segura o grave, enquanto as serras dão o carácter por cima.", "A sine an octave below holds the low end while the saws provide the character above.",
           {{"sub", 0.5}}},
          {"Saturação e volume firme", "Drive and a firm volume", "Saturação faz o baixo ouvir-se em colunas pequenas; sustain total e release curto mantêm as notas firmes.", "Drive makes the bass audible on small speakers; full sustain and a short release keep notes tight.",
           {{"drive", 0.35}, {"aAttack", 0.005}, {"aSustain", 1.0}, {"aRelease", 0.15}, {"width", 0.2}}}}},

        {"Sub limpo", "Clean sub", "Baixo", "Bass",
         "O grave puro que se sente mais do que se ouve: simples, redondo e mono.", "The pure low end you feel more than hear: simple, round and mono.",
         {33}, 1.0, 0.9,
         {{"Um seno", "One sine", "O seno só tem a nota fundamental: é o grave mais limpo e mais forte que existe.", "A sine has only the fundamental: it is the cleanest and strongest low end there is.",
           {{"wave1", 3}, {"voices", 1}, {"cutoff", 18000}}},
          {"Evita cliques", "Avoid clicks", "Ataque e release muito curtos mas não nulos evitam o 'clique' de começar ou acabar a onda a meio.", "Very short but non-zero attack and release avoid the 'click' of starting or stopping a wave mid-cycle.",
           {{"aAttack", 0.005}, {"aDecay", 0.1}, {"aSustain", 1.0}, {"aRelease", 0.12}}},
          {"Harmónicos para colunas pequenas", "Harmonics for small speakers", "Um pouco de saturação acrescenta harmónicos acima do fundamental, que se ouvem mesmo em colunas que não reproduzem graves.", "A little drive adds harmonics above the fundamental, which can be heard even on speakers that cannot reproduce lows.",
           {{"drive", 0.2}}},
          {"Limpa o topo", "Clean the top", "Filtrar acima de 400 Hz remove qualquer brilho inútil e liberta espaço para o resto da mistura.", "Filtering above 400 Hz removes any useless brightness and frees space for the rest of the mix.",
           {{"cutoff", 400}, {"res", 0}}},
          {"Mantém em mono", "Keep it mono", "Graves devem ser mono: evitam problemas de fase e batem com mais força.", "Lows should be mono: they avoid phase problems and hit harder.",
           {{"width", 0}}}}},

        {"Baixo wobble", "Wobble bass", "Baixo", "Bass",
         "O baixo que 'abana' ao ritmo da música: um filtro movido por um LFO sincronizado com o tempo.", "The bass that 'wobbles' in time with the music: a filter moved by a tempo-synced LFO.",
         {33}, 2.0, 1.9,
         {{"Serra e quadrada", "Saw and square", "Uma serra com uma quadrada dá um som cheio e com muito para filtrar.", "A saw with a square gives a full sound with plenty to filter.",
           {{"wave1", 0}, {"wave2", 1}, {"mix2", 0.5}, {"cutoff", 18000}}},
          {"Filtro fechado com ressonância", "Closed filter with resonance", "A ressonância acentua a frequência de corte, por isso quando ela se move ouve-se o 'uóu'.", "Resonance emphasises the cutoff frequency, so when it moves you hear the 'wow'.",
           {{"cutoff", 600}, {"res", 0.45}}},
          {"LFO sincronizado", "Synced LFO", "O LFO abre e fecha o filtro em ciclos de 1/8 de nota: o movimento fica preso ao tempo da música.", "The LFO opens and closes the filter in 1/8-note cycles: the movement locks to the music's tempo.",
           {{"lfoAmt", 0.7}, {"lfoRate", 2}}},
          {"Sub por baixo", "Sub underneath", "O sub mantém o peso constante enquanto o filtro mexe só nos médios.", "The sub keeps the weight constant while the filter moves only the mids.",
           {{"sub", 0.35}}},
          {"Corpo e saturação", "Body and drive", "Três vozes e saturação deixam o wobble mais agressivo e largo, sem perder o grave.", "Three voices and drive make the wobble more aggressive and wide, without losing the low end.",
           {{"voices", 3}, {"detune", 10}, {"drive", 0.3}, {"width", 0.3}}}}},
    };
    return r;
}
}  // namespace

int recipeCount() { return (int) recipes().size(); }

RecipeInfo recipeInfo(int i, Lang l) {
    const auto& r = recipes()[(size_t) std::max(0, std::min(recipeCount() - 1, i))];
    RecipeInfo x;
    x.name = L(l, r.np, r.ne);
    x.category = L(l, r.cp, r.ce);
    x.use = L(l, r.up, r.ue);
    x.notes = r.notes;
    x.rate = r.rate;
    x.length = r.len;
    x.steps = (int) r.steps.size();
    return x;
}

Step recipeStep(int i, int s, Lang l) {
    const auto& r = recipes()[(size_t) std::max(0, std::min(recipeCount() - 1, i))];
    const auto& st = r.steps[(size_t) std::max(0, std::min((int) r.steps.size() - 1, s))];
    return {L(l, st.tp, st.te), L(l, st.wp, st.we), st.set};
}

std::vector<double> stateAfter(int recipe, int step) {
    std::vector<double> v;
    for (const auto& p : params()) v.push_back(p.def);
    const auto& r = recipes()[(size_t) std::max(0, std::min(recipeCount() - 1, recipe))];
    for (int s = 0; s <= step && s < (int) r.steps.size(); ++s)
        for (const auto& kv : r.steps[(size_t) s].set) {
            const int idx = paramIndex(kv.first);
            if (idx >= 0) v[(size_t) idx] = kv.second;
        }
    return v;
}

std::vector<std::string> changedIds(int recipe, int step) {
    std::vector<std::string> out;
    const auto& r = recipes()[(size_t) std::max(0, std::min(recipeCount() - 1, recipe))];
    if (step < 0 || step >= (int) r.steps.size()) return out;
    for (const auto& kv : r.steps[(size_t) step].set) out.push_back(kv.first);
    return out;
}

}  // namespace rec
