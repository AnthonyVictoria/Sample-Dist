#pragma once

#include "PluginProcessor.h"

class OrangeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrangeLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffe85d04));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xfff4ede4));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4ede4));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff241e19));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3128));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff4ede4));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff241e19));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe85d04));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff4ede4));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff1a120c));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float>(x, y, width, height).reduced(6.0f);
        const float radius = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
        const auto centre = b.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        g.setColour(juce::Colour(0xff2c261f));
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 4.0f);

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
        g.setColour(juce::Colour(0xffe85d04));
        g.strokePath(value, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path thumb;
        thumb.addEllipse(-4.0f, -radius + 1.0f, 8.0f, 8.0f);
        g.setColour(juce::Colour(0xfff7f1ea));
        g.fillPath(thumb, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    }
};

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
        auto b = getLocalBounds().toFloat().reduced(10.0f, 8.0f);
        if (samples.size() < 2)
            return;

        juce::Path p;
        const float mid = b.getCentreY() - 18.0f;
        const float halfH = b.getHeight() * 0.34f;
        for (size_t i = 0; i < samples.size(); i += 3)
        {
            float x = b.getX() + (float) i / (float) (samples.size() - 1) * b.getWidth();
            float y = mid - samples[i] * halfH;
            if (i == 0)
                p.startNewSubPath(x, y);
            else
                p.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xffe85d04));
        g.strokePath(p, juce::PathStrokeType(1.8f));
        g.setColour(juce::Colour(0x22f4ede4));
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
    void parentHierarchyChanged() override;

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    void acceptDroppedPath(const juce::String& path);

private:
    void timerCallback() override;
    void loadClicked();
    void styleKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setMode(int index);
    void applyPreset(int id);

    SampleDistortAudioProcessor& processor;
    OrangeLookAndFeel lnf;

    WaveformView waveform;
    juce::TextButton loadButton { "Cargar sample" };
    juce::TextButton modeButtons[3];
    juce::TextButton tabButtons[3];
    juce::TextButton bypassButton { "BYPASS" };
    juce::ComboBox presetBox;
    juce::Label title, subtitle, sampleLabel, curveTitle, meterLabel;
    juce::Slider drive, mix, output, symmetry, rate, ceiling, hp, hpq, lp, lpq;
    juce::Label driveL, mixL, outputL, symmetryL, rateL, ceilingL, hpL, hpqL, lpL, lpqL;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> driveA, mixA, outputA, symmetryA, rateA, ceilingA, hpA, hpqA, lpA, lpqA;
    std::unique_ptr<ButtonAtt> bypassA;

    bool dragOver = false;
    bool dropInstalled = false;
    float shownMeter = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDistortAudioProcessorEditor)
};
