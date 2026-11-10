#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <memory>
#include <vector>

#include "Receitas.h"
#include "SynthEngine.h"

class ReceitasProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster,
                          private juce::AsyncUpdater,
                          private juce::AudioProcessorValueTreeState::Listener {
public:
    ReceitasProcessor();
    ~ReceitasProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Receitas"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- API do editor ----
    juce::AudioProcessorValueTreeState apvts;
    rec::Lang lang() const { return pi("lang") == 0 ? rec::Lang::PT : rec::Lang::EN; }
    int recipe() const { return juce::jlimit(0, rec::recipeCount() - 1, pi("recipe")); }
    int step() const { return pi("step"); }
    void setParam(const juce::String& id, float realValue);
    void applyStep(int recipeIndex, int stepIndex);  // põe todos os botões como depois desse passo (-1 = patch inicial)
    void forTest() { updateTestNotes(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void updateTestNotes();
    float pf(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    int pi(const juce::String& id) const { return (int) std::lround(pf(id)); }

    std::vector<std::atomic<float>*> pp;
    std::atomic<float>* pAudition = nullptr;
    std::atomic<float>* pBpm = nullptr;
    std::atomic<int> testNotes[8];
    std::atomic<int> nTest{0};
    std::atomic<double> testRate{4.0}, testLen{3.5};
    SynthEngine engine;
    struct Ev { int off; bool on; int note; int vel; };
    Ev ev[128];
    int nEv = 0;
    std::vector<int> sounding;
    std::vector<float> lfoBuf;
    bool seqRunning = false, lastAudition = false;
    double expectedP0 = 0, auditionBeat = 0, offBeat = -1;
    double internalBeat = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReceitasProcessor)
};
