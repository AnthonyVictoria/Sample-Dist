#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>

struct SampleData
{
    std::vector<float> mono;       // sample completo (normalizado, mono)
    std::vector<float> table;      // curva de transferencia, 4096 puntos, -1..1
    std::vector<float> symTable;   // versión simétrica (función impar)
    juce::String name;
};

class SampleDistortAudioProcessor : public juce::AudioProcessor
{
public:
    SampleDistortAudioProcessor();
    ~SampleDistortAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadSampleFromFile(const juce::File& file);
    juce::String getSampleName() const;
    juce::File getSampleFile() const { return lastFile; }
    std::vector<float> getDisplayWaveform() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static float readInterp(const std::vector<float>& data, float index);
    float processOne(float x, int channel, int mode, float drive, float symmetry, float rateHz,
                     double osRate, const SampleData* data);

    juce::AudioFormatManager formatManager;
    mutable juce::SpinLock sampleLock;
    std::shared_ptr<SampleData> sampleData;
    juce::File lastFile;

    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };

    double sampleRateHz = 44100.0;

    juce::SmoothedValue<float> driveSm, mixSm, outSm, symSm, rateSm;

    std::array<float, 2> dcX { 0, 0 }, dcY { 0, 0 };
    std::array<float, 2> envPeak { 0, 0 };
    std::array<float, 2> grainPhase { 0, 0 };
    std::array<float, 2> grainInc { 1, 1 };
    std::array<float, 2> grainAmp { 0, 0 };
    std::array<int, 2> lastSign { 1, 1 };
    std::array<int, 2> halfCount { 0, 0 };
    std::array<int, 2> lastHalf { 256, 256 };
    std::array<float, 2> patternPhase { 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDistortAudioProcessor)
};
