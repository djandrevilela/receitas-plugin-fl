#pragma once
// Motor do Receitas: osciladores com unison (polyBLEP), sub, ruído, filtro SVF com envolvente e LFO sincronizado, saturação.
#include <array>
#include <cmath>
#include <cstdint>

struct Patch {
    int wave1 = 1, wave2 = 0, voices = 1;
    float semi2 = 0, mix2 = 0, detune = 0, sub = 0, noise = 0, cutoff = 18000, res = 0.1f, envAmt = 0, fDecay = 0.3f, fSustain = 1;
    float aAtt = 0.005f, aDec = 0.3f, aSus = 1, aRel = 0.2f, drive = 0, width = 0.5f, lfoAmt = 0, volume = 0.7f;
};

class SynthVoice {
public:
    bool active = false, gate = false;
    int note = 60;

    void start(int n, int vel) {
        active = gate = true; note = n; velocity = vel / 127.f;
        stage = 0; level = 0; fenv = 1.f;
        for (int u = 0; u < 7; ++u) { ph1[u] = std::fmod(u * 0.137, 1.0); ph2[u] = std::fmod(u * 0.211 + 0.05, 1.0); }
        subPh = 0;
        for (auto& s : svf) s = 0;
    }
    void stop() { gate = false; }

    void render(float* L, float* R, int n, double sr, const Patch& p, const float* lfo) {
        const double base = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        const int N = std::max(1, std::min(7, p.voices));
        const double semi = std::pow(2.0, p.semi2 / 12.0);
        float gl[7], gr[7];
        double inc1[7], inc2[7];
        for (int u = 0; u < N; ++u) {
            const double pos = N > 1 ? 2.0 * u / (N - 1) - 1.0 : 0.0;
            const double f = base * std::pow(2.0, pos * p.detune / 1200.0);
            inc1[u] = f / sr; inc2[u] = f * semi / sr;
            const double pan = pos * p.width;
            gl[u] = (float) std::cos((pan + 1.0) * 0.7853981634);
            gr[u] = (float) std::sin((pan + 1.0) * 0.7853981634);
        }
        const float norm = 1.f / std::sqrt((float) N);
        const double aInc = 1.0 / std::max(1.0, p.aAtt * sr);
        const double dCoef = std::exp(-3.0 / std::max(1.0, p.aDec * sr)), rCoef = std::exp(-5.0 / std::max(1.0, p.aRel * sr));
        const double fCoef = std::exp(-3.0 / std::max(1.0, p.fDecay * sr));
        const float dgain = 1.f + p.drive * 5.f, dnorm = 1.f / std::sqrt(dgain);
        const double subInc = base * 0.5 / sr;
        const float Q = 0.6f + p.res * 8.f, k = 1.f / Q;
        float a1 = 0, a2 = 0, a3 = 0;
        for (int i = 0; i < n; ++i) {
            // envolventes
            if (gate) {
                if (stage == 0) { level += aInc; if (level >= 1.0) { level = 1.0; stage = 1; } }
                else if (stage == 1) { level = p.aSus + (level - p.aSus) * dCoef; if (std::abs(level - p.aSus) < 1e-4) { level = p.aSus; stage = 2; } }
                fenv = (float) (p.fSustain + (fenv - p.fSustain) * fCoef);
            } else { level *= rCoef; fenv *= (float) rCoef; if (level < 1e-4) { active = false; return; } }
            // osciladores
            float sl = 0, sr_ = 0;
            for (int u = 0; u < N; ++u) {
                ph1[u] += inc1[u]; ph1[u] -= std::floor(ph1[u]);
                float o = osc(p.wave1, ph1[u], inc1[u]);
                if (p.mix2 > 0.001f) {
                    ph2[u] += inc2[u]; ph2[u] -= std::floor(ph2[u]);
                    o = o * (1.f - p.mix2) + osc(p.wave2, ph2[u], inc2[u]) * p.mix2;
                }
                sl += o * gl[u]; sr_ += o * gr[u];
            }
            sl *= norm; sr_ *= norm;
            if (p.sub > 0.001f) {
                subPh += subInc; subPh -= std::floor(subPh);
                const float s = (float) std::sin(6.283185307179586 * subPh) * p.sub;
                sl += s; sr_ += s;
            }
            if (p.noise > 0.001f) { const float nz = noiseNext() * p.noise * 0.4f; sl += nz; sr_ += nz; }
            // filtro (coeficientes a cada 16 amostras)
            if ((i & 15) == 0) {
                const double oct = p.envAmt * 5.0 * fenv + p.lfoAmt * 3.0 * lfo[i];
                double fc = p.cutoff * std::pow(2.0, oct);
                fc = std::max(20.0, std::min(fc, 0.45 * sr));
                const float g = (float) std::tan(3.141592653589793 * fc / sr);
                a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
            }
            sl = lp(0, sl, a1, a2, a3);
            sr_ = lp(2, sr_, a1, a2, a3);
            if (p.drive > 0.001f) { sl = std::tanh(sl * dgain) * dnorm; sr_ = std::tanh(sr_ * dgain) * dnorm; }
            const float g = (float) level * velocity * p.volume * 0.4f;
            L[i] += sl * g; R[i] += sr_ * g;
        }
    }

private:
    float velocity = 1.f;
    int stage = 0;
    double level = 0;
    float fenv = 1.f;
    double ph1[7] = {}, ph2[7] = {}, subPh = 0;
    float svf[4] = {};
    uint32_t ns = 987654321u;

    float noiseNext() { ns = ns * 1664525u + 1013904223u; return (float) (ns >> 8) * (1.0f / 8388608.0f) - 1.0f; }
    float lp(int o, float x, float a1, float a2, float a3) {
        const float v3 = x - svf[o + 1];
        const float v1 = a1 * svf[o] + a2 * v3;
        const float v2 = svf[o + 1] + a2 * svf[o] + a3 * v3;
        svf[o] = 2 * v1 - svf[o]; svf[o + 1] = 2 * v2 - svf[o + 1];
        return v2;
    }
    static float blep(double t, double dt) {
        if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.f;
    }
    static float osc(int wave, double t, double dt) {
        switch (wave) {
            case 0: return (float) (2.0 * t - 1.0) - blep(t, dt);
            case 1: { float v = t < 0.5 ? 1.f : -1.f; v += blep(t, dt); double t2 = t + 0.5; t2 -= std::floor(t2); v -= blep(t2, dt); return v * 0.8f; }
            case 2: return (float) (2.0 * std::abs(2.0 * t - 1.0) - 1.0);
            default: return (float) std::sin(6.283185307179586 * t);
        }
    }
};

class SynthEngine {
public:
    void prepare(double s) { sr = s; for (auto& v : voices) v.active = false; }
    void noteOn(int n, int vel) {
        SynthVoice* v = nullptr;
        for (auto& x : voices) if (!x.active) { v = &x; break; }
        if (!v) v = &voices[next++ % voices.size()];
        v->start(n, vel);
    }
    void noteOff(int n) { for (auto& v : voices) if (v.active && v.gate && v.note == n) v.stop(); }
    void allOff() { for (auto& v : voices) if (v.active) v.stop(); }
    void render(float* L, float* R, int n, const Patch& p, const float* lfo) {
        for (auto& v : voices) if (v.active) v.render(L, R, n, sr, p, lfo);
    }

private:
    std::array<SynthVoice, 10> voices;
    size_t next = 0;
    double sr = 44100;
};
