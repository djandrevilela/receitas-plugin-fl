#include "PluginEditor.h"

#include "Util.h"

using namespace rec;

namespace {
const juce::Colour kBg(0xff14161f), kPanel(0xff1d2130), kEdge(0xff2d3348), kText(0xffe2e8f0), kMuted(0xff8a94a8), kAccent(0xfff6ad55), kTeal(0xff4fd1c5);
const juce::Colour kGroup[kNumGroups] = {juce::Colour(0xff63b3ed), juce::Colour(0xffb794f4), juce::Colour(0xff68d391), juce::Colour(0xfff687b3), juce::Colour(0xfffaf089), juce::Colour(0xff8a94a8)};
}  // namespace

juce::String ReceitasEditor::T(const char* pt, const char* en) const { return U8(L(proc.lang(), pt, en)); }

void ReceitasEditor::styleBox(juce::TextEditor& t) {
    t.setMultiLine(true, true);
    t.setReadOnly(true);
    t.setCaretVisible(false);
    t.setColour(juce::TextEditor::backgroundColourId, kPanel);
    t.setColour(juce::TextEditor::textColourId, kText);
    t.setColour(juce::TextEditor::outlineColourId, kEdge);
    t.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(t);
}

ReceitasEditor::ReceitasEditor(ReceitasProcessor& p) : juce::AudioProcessorEditor(&p), proc(p) {
    setLookAndFeel(&lnf);
    lnf.setColour(juce::ComboBox::backgroundColourId, kPanel);
    lnf.setColour(juce::PopupMenu::backgroundColourId, kPanel);
    lnf.setColour(juce::Slider::thumbColourId, kAccent);
    lnf.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    lnf.setColour(juce::Slider::trackColourId, kAccent.withAlpha(0.5f));
    lnf.setColour(juce::ToggleButton::tickColourId, kAccent);
    lnf.setColour(juce::TextButton::buttonColourId, kPanel);

    langBox.addItemList(juce::StringArray{TR("Portugu\xC3\xAAs (PT)"), "English (UK)"}, 1);
    addAndMakeVisible(langBox);
    cAtt.push_back(std::make_unique<CA>(proc.apvts, "lang", langBox));
    addAndMakeVisible(recipeBox);
    for (int i = 0; i < recipeCount(); ++i) recipeBox.addItem("-", i + 1);
    cAtt.push_back(std::make_unique<CA>(proc.apvts, "recipe", recipeBox));
    addAndMakeVisible(auditionTg);
    bAtt.push_back(std::make_unique<BA>(proc.apvts, "audition", auditionTg));
    bpmS.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmS.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
    addAndMakeVisible(bpmS);
    sAtt.push_back(std::make_unique<SA>(proc.apvts, "bpm", bpmS));
    for (auto* l : {&bpmL, &recipeL, &stepsL, &whyL}) {
        l->setColour(juce::Label::textColourId, kMuted);
        l->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(*l);
    }
    styleBox(descBox);
    styleBox(whyBox);
    whyBox.setFont(juce::FontOptions(15.0f));

    for (int i = 0; i < 6; ++i) {
        addChildComponent(stepBtn[i]);
        stepBtn[i].onClick = [this, i] { proc.applyStep(proc.recipe(), i); };
    }
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);
    addAndMakeVisible(resetBtn);
    prevBtn.onClick = [this] { proc.applyStep(proc.recipe(), std::max(-1, proc.step() - 1)); };
    nextBtn.onClick = [this] {
        const int steps = recipeInfo(proc.recipe(), Lang::EN).steps;
        proc.applyStep(proc.recipe(), std::min(steps - 1, proc.step() + 1));
    };
    resetBtn.onClick = [this] { proc.applyStep(proc.recipe(), -1); };

    const auto& ps = params();
    for (size_t i = 0; i < ps.size(); ++i) {
        auto* c = ctls.add(new Ctl());
        c->idx = (int) i;
        c->label.setJustificationType(juce::Justification::centred);
        c->label.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(c->label);
        if (ps[i].choices > 0) {
            c->combo = std::make_unique<juce::ComboBox>();
            for (int k = 0; k < ps[i].choices; ++k) c->combo->addItem("-", k + 1);
            addAndMakeVisible(*c->combo);
            cAtt.push_back(std::make_unique<CA>(proc.apvts, ps[i].id, *c->combo));
        } else {
            c->slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
            c->slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
            c->slider->setColour(juce::Slider::textBoxTextColourId, kText);
            c->slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible(*c->slider);
            sAtt.push_back(std::make_unique<SA>(proc.apvts, ps[i].id, *c->slider));
        }
    }
    setSize(1120, 760);
    proc.addChangeListener(this);
    refresh();
}
ReceitasEditor::~ReceitasEditor() {
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void ReceitasEditor::refresh() {
    const Lang lg = proc.lang();
    const int r = proc.recipe(), st = proc.step();
    for (int i = 0; i < recipeCount(); ++i) {
        auto info = recipeInfo(i, lg);
        recipeBox.changeItemText(i + 1, U8(info.category + " - " + info.name));
    }
    recipeBox.setText(U8(recipeInfo(r, lg).category + " - " + recipeInfo(r, lg).name), juce::dontSendNotification);
    const auto& ps = params();
    for (auto* c : ctls) {
        const auto& sp = ps[(size_t) c->idx];
        c->label.setText(U8(lg == Lang::PT ? sp.pt : sp.en), juce::dontSendNotification);
        if (c->combo) {
            auto names = choiceNames(sp.id, lg);
            for (size_t k = 0; k < names.size(); ++k) c->combo->changeItemText((int) k + 1, U8(names[k]));
            c->combo->setSelectedId((int) std::lround(proc.apvts.getRawParameterValue(sp.id)->load()) + 1, juce::dontSendNotification);
        }
        c->label.setColour(juce::Label::textColourId, kGroup[sp.group]);
    }
    // botões a mudar neste passo ficam a laranja
    for (auto id : changedIds(r, st)) {
        const int idx = paramIndex(id);
        for (auto* c : ctls) if (c->idx == idx) c->label.setColour(juce::Label::textColourId, kAccent);
    }
    auto info = recipeInfo(r, lg);
    descBox.setText(U8(info.use) + "\n\n" + T("Clica nos passos, um a um, e ouve o som a ganhar forma. Os bot\xC3\xB5" "es a laranja s\xC3\xA3o os que mudaram nesse passo. Depois mexe nos bot\xC3\xB5" "es \xE2\x80\x94 \xC3\xA9 assim que se aprende.",
                                              "Click the steps one by one and hear the sound take shape. Orange knobs are the ones that changed in that step. Then tweak the knobs \xE2\x80\x94 that is how you learn."), false);
    for (int i = 0; i < 6; ++i) {
        stepBtn[i].setVisible(i < info.steps);
        if (i < info.steps) {
            stepBtn[i].setButtonText(juce::String(i + 1) + ".  " + U8(recipeStep(r, i, lg).title));
            stepBtn[i].setColour(juce::TextButton::buttonColourId, i == st ? kAccent.withAlpha(0.35f) : kPanel);
        }
    }
    if (st < 0) whyBox.setText(T("Patch inicial (uma onda simples). Carrega no passo 1 ou em 'Seguinte' para come\xC3\xA7" "ares.", "Initial patch (one plain wave). Press step 1 or 'Next' to begin."), false);
    else {
        auto s = recipeStep(r, st, lg);
        whyBox.setText(juce::String(st + 1) + ". " + U8(s.title) + "\n\n" + T("Porqu\xC3\xAA: ", "Why: ") + U8(s.why), false);
    }
    langBox.setText(lg == Lang::PT ? TR("Portugu\xC3\xAAs (PT)") : "English (UK)", juce::dontSendNotification);
    auditionTg.setButtonText(T("\xE2\x96\xB6 Ouvir o som", "\xE2\x96\xB6 Listen"));
    bpmL.setText("BPM", juce::dontSendNotification);
    recipeL.setText(T("RECEITA", "RECIPE"), juce::dontSendNotification);
    stepsL.setText(T("PASSOS", "STEPS"), juce::dontSendNotification);
    whyL.setText(T("O QUE ACABOU DE ACONTECER E PORQU\xC3\x8A", "WHAT JUST HAPPENED AND WHY"), juce::dontSendNotification);
    prevBtn.setButtonText(T("\xE2\x97\x82 Anterior", "\xE2\x97\x82 Previous"));
    nextBtn.setButtonText(T("Seguinte \xE2\x96\xB8", "Next \xE2\x96\xB8"));
    resetBtn.setButtonText(T("Patch inicial", "Initial patch"));
    repaint();
}

void ReceitasEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kAccent);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("RECEITAS", 12, 8, 140, 24, juce::Justification::left);
}

void ReceitasEditor::resized() {
    recipeL.setBounds(160, 4, 200, 14);
    recipeBox.setBounds(160, 18, 330, 26);
    auditionTg.setBounds(504, 18, 130, 26);
    bpmL.setBounds(640, 18, 34, 26);
    bpmS.setBounds(676, 18, 190, 26);
    langBox.setBounds(960, 18, 148, 26);
    descBox.setBounds(12, 56, 440, 150);
    stepsL.setBounds(12, 214, 440, 16);
    for (int i = 0; i < 6; ++i) stepBtn[i].setBounds(12, 234 + i * 38, 440, 34);
    prevBtn.setBounds(12, 466, 140, 28);
    nextBtn.setBounds(160, 466, 140, 28);
    resetBtn.setBounds(308, 466, 144, 28);
    // botões: grelha de 6 colunas
    const int x0 = 470, y0 = 56, cw = 106, ch = 100;
    int i = 0;
    for (auto* c : ctls) {
        const int col = i % 6, row = i / 6;
        const int x = x0 + col * cw, y = y0 + row * ch;
        c->label.setBounds(x, y, cw - 4, 28);
        if (c->combo) c->combo->setBounds(x + 4, y + 34, cw - 12, 26);
        else c->slider->setBounds(x + 4, y + 26, cw - 12, 72);
        ++i;
    }
    whyL.setBounds(12, 510, 440, 16);
    whyBox.setBounds(12, 530, 1096, 220);
}
