#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

#include "Receitas.h"

using namespace rec;
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); } } while (0)

int main() {
    const auto& ps = params();
    std::set<std::string> ids;
    for (const auto& p : ps) {
        CHECK(ids.insert(p.id).second);
        CHECK(p.max > p.min && p.def >= p.min && p.def <= p.max);
        CHECK(std::string(p.pt).size() > 2 && std::string(p.en).size() > 2 && std::string(p.pt) != std::string(p.en) + "x");
        if (p.logScale) CHECK(p.min > 0);
        if (p.choices) { CHECK(p.min == 0 && p.max == p.choices - 1); CHECK((int) choiceNames(p.id, Lang::PT).size() == p.choices); CHECK((int) choiceNames(p.id, Lang::EN).size() == p.choices); }
    }
    CHECK(recipeCount() >= 6);
    for (int g = 0; g < kNumGroups; ++g) CHECK(!groupName(g, Lang::PT).empty() && groupName(g, Lang::PT) != groupName(g, Lang::EN) + "?");
    for (int r = 0; r < recipeCount(); ++r) {
        auto pt = recipeInfo(r, Lang::PT), en = recipeInfo(r, Lang::EN);
        CHECK(pt.steps == en.steps && pt.steps >= 4 && pt.steps <= 6);
        CHECK(!pt.name.empty() && !en.name.empty() && !pt.use.empty() && pt.use != en.use);
        CHECK(!pt.notes.empty() && pt.rate > 0 && pt.length > 0 && pt.length <= pt.rate);
        for (int n : pt.notes) CHECK(n >= 24 && n <= 96);
        std::set<std::string> everSet;
        for (int s = 0; s < pt.steps; ++s) {
            auto a = recipeStep(r, s, Lang::PT), b = recipeStep(r, s, Lang::EN);
            CHECK(!a.title.empty() && !b.title.empty() && a.title != b.title);
            CHECK(a.why.size() > 40 && b.why.size() > 40 && a.why != b.why);
            CHECK(!a.set.empty() && a.set.size() == b.set.size());
            for (const auto& kv : a.set) {
                int idx = paramIndex(kv.first);
                CHECK(idx >= 0);
                if (idx < 0) continue;
                const auto& sp = ps[(size_t) idx];
                CHECK(kv.second >= sp.min && kv.second <= sp.max);
                if (sp.choices) CHECK(kv.second == std::floor(kv.second));
                everSet.insert(kv.first);
            }
            // o estado acumulado respeita os intervalos e o passo muda mesmo alguma coisa
            auto st = stateAfter(r, s), prev = stateAfter(r, s - 1);
            CHECK(st.size() == ps.size());
            CHECK(st != prev);
            for (size_t i = 0; i < st.size(); ++i) CHECK(st[i] >= ps[i].min && st[i] <= ps[i].max);
            CHECK(!changedIds(r, s).empty());
        }
        CHECK(everSet.size() >= 5);  // cada receita mexe em vários parâmetros
        CHECK(stateAfter(r, -1)[0] == ps[0].def);
        CHECK(stateAfter(r, 99) == stateAfter(r, pt.steps - 1));
        CHECK(stateAfter(r, 2) == stateAfter(r, 2));  // determinismo
    }
    CHECK(lfoRateBeats(0) == 2.0 && lfoRateBeats(4) == 0.125);
    std::printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
