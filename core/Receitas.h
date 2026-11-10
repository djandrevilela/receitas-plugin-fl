#pragma once
// Receitas core: tabela de parâmetros e receitas de sound design passo a passo (bilingue). C++17 puro, determinístico.
#include <string>
#include <utility>
#include <vector>

namespace rec {

enum class Lang { PT, EN };
std::string L(Lang l, const char* pt, const char* en);

enum Group { GOsc = 0, GFilter, GAmp, GFx, GLfo, GOut, kNumGroups };
std::string groupName(int group, Lang);

struct ParamSpec {
    const char* id;
    const char* pt;
    const char* en;
    double min, max, def;
    int choices;  // 0 = contínuo; >0 = número de opções
    bool logScale;
    int group;
};
const std::vector<ParamSpec>& params();
int paramIndex(const std::string& id);  // -1 se não existe
std::vector<std::string> choiceNames(const std::string& id, Lang);  // para parâmetros de escolha
double lfoRateBeats(int choice);  // duração do ciclo do LFO em tempos

struct Step {
    std::string title, why;
    std::vector<std::pair<std::string, double>> set;
};
struct RecipeInfo {
    std::string name, category, use;
    std::vector<int> notes;  // notas MIDI de teste
    double rate = 4, length = 3.5;  // tempos entre disparos e duração da nota de teste
    int steps = 0;
};
int recipeCount();
RecipeInfo recipeInfo(int recipe, Lang);
Step recipeStep(int recipe, int step, Lang);
std::vector<double> stateAfter(int recipe, int step);  // valores alinhados com params(); step < 0 = patch inicial
std::vector<std::string> changedIds(int recipe, int step);

}  // namespace rec
