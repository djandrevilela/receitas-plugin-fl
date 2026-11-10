#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

#include "PluginProcessor.h"

class ReceitasEditor : public juce::AudioProcessorEditor, private juce::ChangeListener {
public:
    explicit ReceitasEditor(ReceitasProcessor&);
    ~ReceitasEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void refresh();
    juce::String T(const char* pt, const char* en) const;
    void styleBox(juce::TextEditor&);

    struct Ctl {
        int idx = 0;
        juce::Label label;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
    };

    ReceitasProcessor& proc;
    juce::LookAndFeel_V4 lnf;
    juce::ComboBox langBox, recipeBox;
    juce::ToggleButton auditionTg;
    juce::Slider bpmS;
    juce::Label bpmL, recipeL, stepsL, whyL;
    juce::TextEditor descBox, whyBox;
    juce::TextButton stepBtn[6], prevBtn, nextBtn, resetBtn;
    juce::OwnedArray<Ctl> ctls;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
    std::vector<std::unique_ptr<SA>> sAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReceitasEditor)
};
