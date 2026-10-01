#pragma once

#include "PluginProcessor.h"

class WaveformView : public juce::Component
{
public:
    void setSamples(std::vector<float> s)
    {
        samples = std::move(s);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(8.0f);
        g.setColour(juce::Colour(0xff1a1612));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        g.setColour(juce::Colour(0xff3a3128));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);

        if (samples.size() < 2)
            return;

        juce::Path p;
        const float mid = b.getCentreY();
        const float halfH = b.getHeight() * 0.42f;
        for (size_t i = 0; i < samples.size(); i += 4)
        {
            float x = b.getX() + (float) i / (float) (samples.size() - 1) * b.getWidth();
            float y = mid - samples[i] * halfH;
            if (i == 0)
                p.startNewSubPath(x, y);
            else
                p.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xffe85d04));
        g.strokePath(p, juce::PathStrokeType(1.6f));

        g.setColour(juce::Colour(0x33f4ede4));
        g.drawHorizontalLine((int) mid, b.getX(), b.getRight());
    }

private:
    std::vector<float> samples;
};

class SampleDistortAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::FileDragAndDropTarget,
                                          private juce::Timer
{
public:
    explicit SampleDistortAudioProcessorEditor(SampleDistortAudioProcessor&);
    ~SampleDistortAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void loadClicked();
    void styleKnob(juce::Slider& s);

    SampleDistortAudioProcessor& processor;

    WaveformView waveform;
    juce::TextButton loadButton { "Cargar sample" };
    juce::Label title, hint, sampleLabel;
    juce::ComboBox modeBox;

    juce::Slider drive, mix, output, symmetry, rate;
    juce::Label driveL, mixL, outputL, symmetryL, rateL;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAtt> driveA, mixA, outputA, symmetryA, rateA;
    std::unique_ptr<ComboAtt> modeA;

    bool dragOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDistortAudioProcessorEditor)
};
