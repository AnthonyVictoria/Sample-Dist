#include "PluginEditor.h"
#include "MacDropTarget.h"

SampleDistortAudioProcessorEditor::SampleDistortAudioProcessorEditor(SampleDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lnf);
    setSize(980, 640);

    title.setText("ShaqkeeDistort", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(26.0f)));
    title.setColour(juce::Label::textColourId, juce::Colour(0xfff4ede4));
    addAndMakeVisible(title);

    subtitle.setText("Distorsion basada en muestras", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(0xffb7aa9c));
    addAndMakeVisible(subtitle);

    curveTitle.setText("CURVA DE DISTORSION", juce::dontSendNotification);
    curveTitle.setColour(juce::Label::textColourId, juce::Colour(0xffb7aa9c));
    addAndMakeVisible(curveTitle);

    sampleLabel.setText(processor.getSampleName(), juce::dontSendNotification);
    sampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe85d04));
    addAndMakeVisible(sampleLabel);

    meterLabel.setJustificationType(juce::Justification::centredRight);
    meterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe85d04));
    addAndMakeVisible(meterLabel);

    addAndMakeVisible(waveform);
    waveform.setSamples(processor.getDisplayWaveform());

    loadButton.onClick = [this] { loadClicked(); };
    addAndMakeVisible(loadButton);

    const char* tabs[] = { "DISTORT", "FORMA", "MOD" };
    for (int i = 0; i < 3; ++i)
    {
        tabButtons[i].setButtonText(tabs[i]);
        tabButtons[i].setClickingTogglesState(false);
        tabButtons[i].onClick = [this, i] { setMode(i); };
        addAndMakeVisible(tabButtons[i]);
    }

    const char* modes[] = { "Curva", "Waveset", "Patron" };
    for (int i = 0; i < 3; ++i)
    {
        modeButtons[i].setButtonText(modes[i]);
        modeButtons[i].onClick = [this, i] { setMode(i); };
        addAndMakeVisible(modeButtons[i]);
    }

    presetBox.addItem("Init", 1);
    presetBox.addItem("Curva suave", 2);
    presetBox.addItem("Waveset", 3);
    presetBox.addItem("Patron", 4);
    presetBox.addItem("Filtro telefono", 5);
    presetBox.setTextWhenNothingSelected("Preset");
    presetBox.onChange = [this] { applyPreset(presetBox.getSelectedId()); };
    addAndMakeVisible(presetBox);

    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    bypassA = std::make_unique<ButtonAtt>(processor.apvts, "bypass", bypassButton);

    auto hook = [this](juce::Slider& s, juce::Label& l, const juce::String& name, const char* param, std::unique_ptr<SliderAtt>& att)
    {
        styleKnob(s, l, name);
        att = std::make_unique<SliderAtt>(processor.apvts, param, s);
    };
    hook(drive, driveL, "DRIVE", "drive", driveA);
    hook(mix, mixL, "MIX", "mix", mixA);
    hook(output, outputL, "OUTPUT", "output", outputA);
    hook(symmetry, symmetryL, "SIMETRIA", "symmetry", symmetryA);
    hook(rate, rateL, "RATE", "rate", rateA);
    hook(ceiling, ceilingL, "LIMIAR", "ceiling", ceilingA);
    hook(hp, hpL, "HIGH PASS", "hp", hpA);
    hook(hpq, hpqL, "HP Q", "hpq", hpqA);
    hook(lp, lpL, "LOW PASS", "lp", lpA);
    hook(lpq, lpqL, "LP Q", "lpq", lpqA);

    startTimerHz(20);
}

SampleDistortAudioProcessorEditor::~SampleDistortAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void SampleDistortAudioProcessorEditor::styleKnob(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
    addAndMakeVisible(s);
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, juce::Colour(0xffb7aa9c));
    addAndMakeVisible(l);
}

void SampleDistortAudioProcessorEditor::setMode(int index)
{
    if (auto* param = processor.apvts.getParameter("mode"))
        param->setValueNotifyingHost(param->convertTo0to1((float) index));
    for (int i = 0; i < 3; ++i)
    {
        tabButtons[i].setToggleState(i == index, juce::dontSendNotification);
        modeButtons[i].setToggleState(i == index, juce::dontSendNotification);
    }
}

void SampleDistortAudioProcessorEditor::applyPreset(int id)
{
    if (id <= 0)
        return;
    auto set = [this](const char* name, float value)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(processor.apvts.getParameter(name)))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    set("drive", 4.0f);
    set("mix", 1.0f);
    set("output", 0.0f);
    set("symmetry", 0.75f);
    set("rate", 2.0f);
    set("ceiling", 1.0f);
    set("hp", 20.0f);
    set("hpq", 0.707f);
    set("lp", 20000.0f);
    set("lpq", 0.707f);
    setMode(0);
    if (id == 2) { set("drive", 8.0f); set("symmetry", 1.0f); }
    if (id == 3) { set("drive", 6.0f); setMode(1); }
    if (id == 4) { set("drive", 5.0f); set("rate", 4.0f); setMode(2); }
    if (id == 5) { set("hp", 300.0f); set("lp", 2800.0f); set("hpq", 0.8f); set("lpq", 0.8f); }
}

void SampleDistortAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0e0c0a));
    g.setColour(juce::Colour(0xff1b1714));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f), 14.0f);
    g.setColour(juce::Colour(0xffe85d04));
    g.fillRoundedRectangle(28.0f, 28.0f, 8.0f, 28.0f, 2.0f);

    auto curve = waveform.getBounds().toFloat();
    g.setColour(juce::Colour(0xff14110e));
    g.fillRoundedRectangle(curve, 10.0f);
    g.setColour(dragOver ? juce::Colour(0xffe85d04) : juce::Colour(0xff3a3128));
    g.drawRoundedRectangle(curve, 10.0f, dragOver ? 2.0f : 1.0f);

    auto drop = curve.removeFromBottom(78.0f).reduced(16.0f, 8.0f);
    g.setColour(juce::Colour(0xff241c16));
    g.fillRoundedRectangle(drop, 8.0f);
    g.setColour(juce::Colour(0xff5a4b3e));
    g.drawRoundedRectangle(drop, 8.0f, 1.0f);
    g.setColour(juce::Colour(0xffe85d04));
    g.setFont(juce::Font(juce::FontOptions(18.0f)));
    g.drawFittedText("Arrastra un sample", drop.removeFromTop(34.0f).toNearestInt(), juce::Justification::centred, 1);
    g.setColour(juce::Colour(0xffb7aa9c));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawFittedText("Desde FL Studio o el Finder. Solo afecta a la distorsion.", drop.toNearestInt(), juce::Justification::centred, 1);

    auto meter = meterLabel.getBounds().toFloat().translated(-150.0f, 2.0f).withWidth(140.0f).withHeight(10.0f);
    g.setColour(juce::Colour(0xff2c261f));
    g.fillRoundedRectangle(meter, 3.0f);
    const float norm = juce::jlimit(0.0f, 1.0f, (shownMeter + 48.0f) / 48.0f);
    g.setColour(shownMeter > -1.0f ? juce::Colour(0xffff4d2e) : juce::Colour(0xffe85d04));
    g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * norm), 3.0f);
}

void SampleDistortAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(24);
    auto header = b.removeFromTop(58);
    title.setBounds(header.removeFromTop(32).withTrimmedLeft(18));
    subtitle.setBounds(header.withTrimmedLeft(18));
    presetBox.setBounds(getWidth() - 220, 28, 180, 28);

    auto tabs = b.removeFromTop(32);
    tabs.removeFromLeft(18);
    for (auto& tab : tabButtons)
        tab.setBounds(tabs.removeFromLeft(110).reduced(4, 2));

    b.removeFromTop(8);
    auto body = b.removeFromTop(390);
    auto left = body.removeFromLeft(150);
    auto right = body.removeFromRight(190);
    body.removeFromLeft(8);
    body.removeFromRight(8);

    curveTitle.setBounds(body.removeFromTop(22));
    sampleLabel.setBounds(body.removeFromTop(20));
    waveform.setBounds(body);

    auto place = [](juce::Rectangle<int> col, juce::Label& l, juce::Slider& s)
    {
        auto cell = col.removeFromTop(128);
        l.setBounds(cell.removeFromTop(16));
        s.setBounds(cell);
    };
    place(left, driveL, drive);
    place(left, mixL, mix);
    place(left, outputL, output);

    auto modes = right.removeFromTop(78);
    modeButtons[0].setBounds(modes.removeFromTop(36).reduced(4));
    auto row = modes;
    modeButtons[1].setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(4));
    modeButtons[2].setBounds(row.reduced(4));
    place(right, symmetryL, symmetry);
    place(right, rateL, rate);

    b.removeFromTop(8);
    auto filters = b.removeFromTop(128);
    const int w = filters.getWidth() / 5;
    auto slot = [&](juce::Label& l, juce::Slider& s)
    {
        auto col = filters.removeFromLeft(w);
        l.setBounds(col.removeFromTop(16));
        s.setBounds(col);
    };
    slot(hpL, hp);
    slot(hpqL, hpq);
    slot(lpL, lp);
    slot(lpqL, lpq);
    slot(ceilingL, ceiling);

    auto footer = b.removeFromBottom(32);
    bypassButton.setBounds(footer.removeFromLeft(120));
    loadButton.setBounds(footer.removeFromLeft(150).reduced(8, 0));
    meterLabel.setBounds(footer.removeFromRight(90));
}

void SampleDistortAudioProcessorEditor::parentHierarchyChanged()
{
    if (dropInstalled)
        return;
    if (auto* peer = getPeer())
    {
        installMacDropTarget(peer->getNativeHandle(), [this](const std::string& path)
        {
            acceptDroppedPath(juce::String(path));
        });
        dropInstalled = true;
    }
}

void SampleDistortAudioProcessorEditor::acceptDroppedPath(const juce::String& path)
{
    juce::String cleaned = path.trim().unquoted().replace("\r", "").replace("\n", "");
    if (cleaned.contains(","))
        cleaned = cleaned.upToFirstOccurrenceOf(",", false, false).trim();
    juce::File file(cleaned);
    if (!file.existsAsFile())
        file = juce::File(juce::URL::removeEscapeChars(cleaned));
    if (file.existsAsFile() && processor.loadSampleFromFile(file))
    {
        sampleLabel.setText(processor.getSampleName(), juce::dontSendNotification);
        waveform.setSamples(processor.getDisplayWaveform());
    }
    dragOver = false;
    repaint();
}

void SampleDistortAudioProcessorEditor::fileDragEnter(const juce::StringArray&, int, int)
{
    dragOver = true;
    repaint();
}

void SampleDistortAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    dragOver = false;
    repaint();
}

void SampleDistortAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (auto& f : files)
    {
        acceptDroppedPath(f);
        if (processor.getSampleName().isNotEmpty())
            break;
    }
}

void SampleDistortAudioProcessorEditor::loadClicked()
{
    auto chooser = std::make_shared<juce::FileChooser>("Elige un sample", juce::File(),
                                                        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser](const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile())
                                  acceptDroppedPath(f.getFullPathName());
                          });
}

void SampleDistortAudioProcessorEditor::timerCallback()
{
    auto name = processor.getSampleName();
    if (name != sampleLabel.getText())
        sampleLabel.setText(name, juce::dontSendNotification);

    const int mode = (int) processor.apvts.getRawParameterValue("mode")->load();
    for (int i = 0; i < 3; ++i)
    {
        tabButtons[i].setToggleState(i == mode, juce::dontSendNotification);
        modeButtons[i].setToggleState(i == mode, juce::dontSendNotification);
    }

    shownMeter = shownMeter * 0.8f + processor.getMeterDb() * 0.2f;
    meterLabel.setText(juce::String(shownMeter, 1) + " dB", juce::dontSendNotification);
    repaint();
}
