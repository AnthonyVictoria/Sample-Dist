#include "PluginEditor.h"

SampleDistortAudioProcessorEditor::SampleDistortAudioProcessorEditor(SampleDistortAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(760, 460);

    title.setText("SAMPLE DISTORT", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(22.0f)));
    title.setColour(juce::Label::textColourId, juce::Colour(0xfff4ede4));
    addAndMakeVisible(title);

    hint.setText("Arrastra un WAV, AIFF, FLAC o MP3. Ese audio se convierte en la distorsion.",
                 juce::dontSendNotification);
    hint.setColour(juce::Label::textColourId, juce::Colour(0xffb7aa9c));
    addAndMakeVisible(hint);

    sampleLabel.setText(processor.getSampleName(), juce::dontSendNotification);
    sampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe85d04));
    addAndMakeVisible(sampleLabel);

    addAndMakeVisible(waveform);
    waveform.setSamples(processor.getDisplayWaveform());

    loadButton.onClick = [this] { loadClicked(); };
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a241e));
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff4ede4));
    addAndMakeVisible(loadButton);

    modeBox.addItem("Curva", 1);
    modeBox.addItem("Waveset", 2);
    modeBox.addItem("Patron", 3);
    addAndMakeVisible(modeBox);
    modeA = std::make_unique<ComboAtt>(processor.apvts, "mode", modeBox);

    auto setup = [this](juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& suffix)
    {
        styleKnob(s);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xffb7aa9c));
        addAndMakeVisible(l);
    };

    setup(drive, driveL, "DRIVE", " x");
    setup(mix, mixL, "MIX", "");
    setup(output, outputL, "OUTPUT", " dB");
    setup(symmetry, symmetryL, "SIMETRIA", "");
    setup(rate, rateL, "RATE", " Hz");

    mix.setNumDecimalPlacesToDisplay(2);
    symmetry.setNumDecimalPlacesToDisplay(2);

    driveA = std::make_unique<SliderAtt>(processor.apvts, "drive", drive);
    mixA = std::make_unique<SliderAtt>(processor.apvts, "mix", mix);
    outputA = std::make_unique<SliderAtt>(processor.apvts, "output", output);
    symmetryA = std::make_unique<SliderAtt>(processor.apvts, "symmetry", symmetry);
    rateA = std::make_unique<SliderAtt>(processor.apvts, "rate", rate);

    startTimerHz(15);
}

SampleDistortAudioProcessorEditor::~SampleDistortAudioProcessorEditor() = default;

void SampleDistortAudioProcessorEditor::styleKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffe85d04));
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff4ede4));
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4ede4));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SampleDistortAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12100e));
    g.setColour(juce::Colour(0xff1c1814));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 12.0f);

    auto drop = waveform.getBounds().toFloat().expanded(6.0f);
    g.setColour(dragOver ? juce::Colour(0xffe85d04) : juce::Colour(0xff3a3128));
    g.drawRoundedRectangle(drop, 10.0f, dragOver ? 2.0f : 1.0f);
}

void SampleDistortAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(22);
    title.setBounds(b.removeFromTop(28));
    hint.setBounds(b.removeFromTop(22));
    b.removeFromTop(8);

    auto top = b.removeFromTop(36);
    loadButton.setBounds(top.removeFromLeft(150));
    top.removeFromLeft(10);
    modeBox.setBounds(top.removeFromLeft(160).reduced(0, 4));
    top.removeFromLeft(10);
    sampleLabel.setBounds(top);

    b.removeFromTop(8);
    waveform.setBounds(b.removeFromTop(180));

    b.removeFromTop(16);
    auto knobs = b.removeFromTop(150);
    const int w = knobs.getWidth() / 5;
    auto place = [&](juce::Slider& s, juce::Label& l)
    {
        auto col = knobs.removeFromLeft(w);
        l.setBounds(col.removeFromTop(18));
        s.setBounds(col);
    };
    place(drive, driveL);
    place(mix, mixL);
    place(output, outputL);
    place(symmetry, symmetryL);
    place(rate, rateL);
}

bool SampleDistortAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".aif" || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }
    return false;
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
    dragOver = false;
    for (auto& f : files)
    {
        juce::File file(f);
        if (processor.loadSampleFromFile(file))
        {
            sampleLabel.setText(processor.getSampleName(), juce::dontSendNotification);
            waveform.setSamples(processor.getDisplayWaveform());
            break;
        }
    }
    repaint();
}

void SampleDistortAudioProcessorEditor::loadClicked()
{
    auto chooser = std::make_shared<juce::FileChooser>("Elige un sample", juce::File(),
                                                        "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser](const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile() && processor.loadSampleFromFile(f))
                              {
                                  sampleLabel.setText(processor.getSampleName(), juce::dontSendNotification);
                                  waveform.setSamples(processor.getDisplayWaveform());
                              }
                          });
}

void SampleDistortAudioProcessorEditor::timerCallback()
{
    auto name = processor.getSampleName();
    if (name != sampleLabel.getText())
        sampleLabel.setText(name, juce::dontSendNotification);
}
