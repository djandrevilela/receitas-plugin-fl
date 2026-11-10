// Teste de áudio offline: cada passo de cada receita deve soar (sem NaN, com som, sem clipping).
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    ReceitasProcessor proc;
    const double sr = 44100.0;
    const int bs = 512;
    proc.setPlayConfigDetails(0, 2, sr, bs);
    proc.prepareToPlay(sr, bs);
    juce::AudioBuffer<float> buf(2, bs);
    int fails = 0;
    std::printf("%-24s %-5s %7s %8s\n", "receita", "passo", "pico", "rms");
    for (int r = 0; r < rec::recipeCount(); ++r) {
        proc.setParam("recipe", (float) r);
        proc.forTest();
        for (int s = 0; s < rec::recipeInfo(r, rec::Lang::EN).steps; ++s) {
            proc.applyStep(r, s);
            proc.apvts.getParameter("audition")->setValueNotifyingHost(0.f);
            proc.apvts.getParameter("audition")->setValueNotifyingHost(1.f);
            double peak = 0, ss = 0;
            long cnt = 0;
            bool bad = false;
            for (int b = 0; b < 700; ++b) {  // ~8 s
                juce::MidiBuffer mb;
                buf.clear();
                proc.processBlock(buf, mb);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < bs; ++i) {
                        float x = buf.getSample(ch, i);
                        if (!std::isfinite(x)) bad = true;
                        peak = std::max(peak, (double) std::abs(x));
                        ss += x * x; ++cnt;
                    }
            }
            const double rms = std::sqrt(ss / (double) cnt);
            const bool ok = !bad && peak > 0.02 && peak < 1.0 && rms > 0.004;
            std::printf("%-24s %-5d %7.3f %8.4f %s\n", rec::recipeInfo(r, rec::Lang::EN).name.c_str(), s + 1, peak, rms, ok ? "" : "<-- PROBLEMA");
            if (!ok) ++fails;
            proc.apvts.getParameter("audition")->setValueNotifyingHost(0.f);
            for (int b = 0; b < 400; ++b) { juce::MidiBuffer mb; buf.clear(); proc.processBlock(buf, mb); }  // deixa a cauda acabar
        }
    }
    std::printf(fails == 0 ? "Tudo OK.\n" : "%d problema(s).\n", fails);
    return fails == 0 ? 0 : 1;
}
