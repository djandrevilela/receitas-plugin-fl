#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

#include "PluginEditor.h"
#include "Util.h"

using namespace rec;

juce::AudioProcessorValueTreeState::ParameterLayout ReceitasProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<AudioParameterChoice>(ParameterID{"lang", 1}, "Language", StringArray{TR("Portugu\xC3\xAAs (PT)"), "English (UK)"}, 0));
    l.add(std::make_unique<AudioParameterInt>(ParameterID{"recipe", 1}, "Recipe", 0, recipeCount() - 1, 0));
    l.add(std::make_unique<AudioParameterInt>(ParameterID{"step", 1}, "Step", -1, 5, -1));
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"audition", 1}, "Listen", false));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID{"bpm", 1}, "BPM", NormalisableRange<float>(60.f, 200.f), 120.f));
    for (const auto& p : params()) {
        const String id = p.id, name = p.en;
        if (p.choices > 0) {
            StringArray items;
            for (auto& s : choiceNames(p.id, Lang::EN)) items.add(U8(s));
            l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, items, (int) p.def));
        } else if (id == "voices" || id == "semi2") {
            l.add(std::make_unique<AudioParameterInt>(ParameterID{id, 1}, name, (int) p.min, (int) p.max, (int) p.def));
        } else {
            NormalisableRange<float> r((float) p.min, (float) p.max);
            if (p.logScale) r.setSkewForCentre((float) std::sqrt(p.min * p.max));
            l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, r, (float) p.def));
        }
    }
    return l;
}

ReceitasProcessor::ReceitasProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "RECEITAS", createLayout()) {
    for (const auto& p : params()) pp.push_back(apvts.getRawParameterValue(p.id));
    pAudition = apvts.getRawParameterValue("audition");
    pBpm = apvts.getRawParameterValue("bpm");
    apvts.addParameterListener("recipe", this);
    sounding.reserve(16);
    lfoBuf.assign(8192, 0.f);
    updateTestNotes();
}
ReceitasProcessor::~ReceitasProcessor() {
    cancelPendingUpdate();
    apvts.removeParameterListener("recipe", this);
}

void ReceitasProcessor::prepareToPlay(double sr, int bs) {
    engine.prepare(sr);
    sounding.clear();
    seqRunning = false;
    lfoBuf.assign((size_t) std::max(bs, 8192), 0.f);
}

void ReceitasProcessor::setParam(const juce::String& id, float v) {
    if (auto* p = apvts.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}

void ReceitasProcessor::applyStep(int r, int s) {
    const auto st = stateAfter(r, s);
    const auto& ps = params();
    for (size_t i = 0; i < ps.size(); ++i) setParam(ps[i].id, (float) st[i]);
    setParam("step", (float) s);
}

void ReceitasProcessor::parameterChanged(const juce::String&, float) { triggerAsyncUpdate(); }
void ReceitasProcessor::handleAsyncUpdate() {
    updateTestNotes();
    sendChangeMessage();
}
void ReceitasProcessor::updateTestNotes() {
    const auto info = recipeInfo(recipe(), Lang::EN);
    nTest = 0;
    for (size_t i = 0; i < info.notes.size() && i < 8; ++i) testNotes[i] = info.notes[i];
    nTest = (int) std::min<size_t>(8, info.notes.size());
    testRate = info.rate;
    testLen = info.length;
}

void ReceitasProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    bool playing = false, hasPpq = false;
    double ppq = 0, bpm = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; hasPpq = true; }
            if (auto b = pos->getBpm()) bpm = *b;
        }
    if (bpm <= 0) bpm = (double) pBpm->load();
    const double bps = bpm / 60.0 / sr;

    Patch p;
    auto V = [&](const char* id) { return pp[(size_t) paramIndex(id)]->load(); };
    p.wave1 = (int) std::lround(V("wave1")); p.wave2 = (int) std::lround(V("wave2")); p.voices = (int) std::lround(V("voices"));
    p.semi2 = V("semi2"); p.mix2 = V("mix2"); p.detune = V("detune"); p.sub = V("sub"); p.noise = V("noise");
    p.cutoff = V("cutoff"); p.res = V("res"); p.envAmt = V("envAmt"); p.fDecay = V("fDecay"); p.fSustain = V("fSustain");
    p.aAtt = V("aAttack"); p.aDec = V("aDecay"); p.aSus = V("aSustain"); p.aRel = V("aRelease");
    p.drive = V("drive"); p.width = V("width"); p.lfoAmt = V("lfoAmt"); p.volume = V("volume");
    const double rateBeats = lfoRateBeats((int) std::lround(V("lfoRate")));

    // relógio: anfitrião a tocar, ou "Ouvir" interno
    const bool audition = pAudition->load() > 0.5f;
    bool useClock = false;
    double p0 = 0;
    if (playing && hasPpq) { useClock = false; }  // com o FL a tocar só o teclado/MIDI toca; o LFO segue o transporte
    if (audition) {
        if (!lastAudition) { auditionBeat = 0; offBeat = -1; }
        useClock = true;
        p0 = auditionBeat;
        auditionBeat += n * bps;
    }
    lastAudition = audition;
    const double p1 = p0 + n * bps;
    nEv = 0;
    auto addEv = [&](int off, bool on, int note, int vel) {
        if (nEv >= 128) return;
        Ev e{juce::jlimit(0, n - 1, off), on, note, vel};
        int j = nEv++;
        while (j > 0 && ev[j - 1].off > e.off) { ev[j] = ev[j - 1]; --j; }
        ev[j] = e;
    };
    auto releaseAll = [&](int off) { for (int nt : sounding) addEv(off, false, nt, 0); sounding.clear(); };
    if (useClock) {
        const double rate = testRate.load(), len = testLen.load();
        if (!seqRunning) { seqRunning = true; }
        if (offBeat >= 0 && offBeat < p1 && !sounding.empty()) { releaseAll((int) ((std::max(offBeat, p0) - p0) / bps)); offBeat = -1; }
        long long k = (long long) std::ceil(p0 / rate - 1e-9);
        for (; (double) k * rate < p1; ++k) {
            const double t = (double) k * rate;
            const int off = (int) ((t - p0) / bps);
            releaseAll(off);
            for (int i = 0; i < nTest.load(); ++i) { addEv(off, true, testNotes[i].load(), 96); sounding.push_back(testNotes[i].load()); }
            offBeat = t + len;
            if (offBeat < p1) { releaseAll((int) ((offBeat - p0) / bps)); offBeat = -1; }
        }
    } else if (seqRunning) {
        releaseAll(0);
        seqRunning = false;
    }
    for (const auto meta : midi) {
        auto m = meta.getMessage();
        if (m.isNoteOn()) addEv(meta.samplePosition, true, m.getNoteNumber(), m.getVelocity());
        else if (m.isNoteOff()) addEv(meta.samplePosition, false, m.getNoteNumber(), 0);
    }
    // LFO sincronizado com o tempo (anfitrião ou relógio interno)
    const double lfoBeat0 = (playing && hasPpq) ? ppq : (useClock ? p0 : internalBeat);
    internalBeat += n * bps;
    if ((int) lfoBuf.size() < n) lfoBuf.assign((size_t) n, 0.f);
    for (int i = 0; i < n; ++i) lfoBuf[(size_t) i] = (float) std::sin(6.283185307179586 * (lfoBeat0 + i * bps) / rateBeats);

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;
    int pos = 0;
    for (int i = 0; i < nEv; ++i) {
        if (ev[i].off > pos) { engine.render(L + pos, R + pos, ev[i].off - pos, p, lfoBuf.data() + pos); pos = ev[i].off; }
        if (ev[i].on) engine.noteOn(ev[i].note, ev[i].vel); else engine.noteOff(ev[i].note);
    }
    if (n > pos) engine.render(L + pos, R + pos, n - pos, p, lfoBuf.data() + pos);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = std::tanh(d[i]);
    }
}

void ReceitasProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void ReceitasProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    cancelPendingUpdate();
    updateTestNotes();
    sendChangeMessage();
}

juce::AudioProcessorEditor* ReceitasProcessor::createEditor() { return new ReceitasEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReceitasProcessor(); }
