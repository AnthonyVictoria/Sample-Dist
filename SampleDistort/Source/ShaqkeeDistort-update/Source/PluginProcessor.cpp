#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kTableSize = 4096;
constexpr int kMaxSample = 192000; // ~4 s a 48 kHz

juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float>(1.0f, 40.0f, 0.01f, 0.45f), 4.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "symmetry", "Simetria",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "rate", "Rate",
        juce::NormalisableRange<float>(0.05f, 20.0f, 0.001f, 0.4f), 2.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Modo",
        juce::StringArray { "Curva", "Waveset", "Patron" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Limiar",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.001f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "hp", "High pass",
        juce::NormalisableRange<float>(20.0f, 18000.0f, 0.01f, 0.3f), 20.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "hpq", "High pass Q",
        juce::NormalisableRange<float>(0.3f, 12.0f, 0.001f, 0.4f), 0.707f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lp", "Low pass",
        juce::NormalisableRange<float>(40.0f, 20000.0f, 0.01f, 0.3f), 20000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lpq", "Low pass Q",
        juce::NormalisableRange<float>(0.3f, 12.0f, 0.001f, 0.4f), 0.707f));

    layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    return layout;
}
}

SampleDistortAudioProcessor::SampleDistortAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", makeLayout())
{
    formatManager.registerBasicFormats();

    auto data = std::make_shared<SampleData>();
    data->name = "Soft clip (sin sample)";
    data->table.resize(kTableSize);
    data->symTable.resize(kTableSize);
    for (int i = 0; i < kTableSize; ++i)
    {
        float x = (float) i / (float) (kTableSize - 1) * 2.0f - 1.0f;
        float y = std::tanh(x * 2.5f);
        data->table[(size_t) i] = y;
        data->symTable[(size_t) i] = y;
    }
    sampleData = data;
}

SampleDistortAudioProcessor::~SampleDistortAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout SampleDistortAudioProcessor::createLayout()
{
    return makeLayout();
}

bool SampleDistortAudioProcessor::loadSampleFromFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const int channels = (int) reader->numChannels;
    const int length = (int) juce::jmin((juce::int64) kMaxSample, reader->lengthInSamples);

    juce::AudioBuffer<float> tmp(juce::jmax(1, channels), length);
    reader->read(&tmp, 0, length, 0, true, true);

    auto data = std::make_shared<SampleData>();
    data->name = file.getFileName();
    data->mono.resize((size_t) length);

    float peak = 0.0f;
    for (int i = 0; i < length; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < tmp.getNumChannels(); ++ch)
            s += tmp.getSample(ch, i);
        s /= (float) tmp.getNumChannels();
        data->mono[(size_t) i] = s;
        peak = juce::jmax(peak, std::abs(s));
    }

    if (peak < 1.0e-6f)
        return false;

    for (auto& s : data->mono)
        s /= peak;

    data->table.resize(kTableSize);
    data->symTable.resize(kTableSize);

    const float n = (float) data->mono.size();
    for (int i = 0; i < kTableSize; ++i)
    {
        float pos = (float) i / (float) (kTableSize - 1) * (n - 1.0f);
        data->table[(size_t) i] = readInterp(data->mono, pos);
    }

    // Función impar: el sample define la mitad positiva y se espeja.
    for (int i = 0; i < kTableSize; ++i)
    {
        float x = (float) i / (float) (kTableSize - 1); // 0..1
        float pos = x * (n - 1.0f);
        float mag = std::abs(readInterp(data->mono, pos));
        float bipolar = (float) i / (float) (kTableSize - 1) * 2.0f - 1.0f;
        data->symTable[(size_t) i] = std::copysign(mag, bipolar);
    }

    {
        juce::SpinLock::ScopedLockType sl(sampleLock);
        sampleData = data;
        lastFile = file;
    }
    return true;
}

juce::String SampleDistortAudioProcessor::getSampleName() const
{
    juce::SpinLock::ScopedLockType sl(sampleLock);
    return sampleData != nullptr ? sampleData->name : juce::String();
}

std::vector<float> SampleDistortAudioProcessor::getDisplayWaveform() const
{
    juce::SpinLock::ScopedLockType sl(sampleLock);
    if (sampleData == nullptr || sampleData->table.empty())
        return {};
    return sampleData->table;
}

void SampleDistortAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    oversampling.initProcessing((size_t) samplesPerBlock);
    setLatencySamples((int) oversampling.getLatencyInSamples());

    const double osRate = sampleRate * oversampling.getOversamplingFactor();
    driveSm.reset(osRate, 0.02);
    mixSm.reset(sampleRate, 0.02);
    outSm.reset(sampleRate, 0.02);
    symSm.reset(osRate, 0.02);
    rateSm.reset(osRate, 0.05);
    ceilSm.reset(sampleRate, 0.02);
    hpSm.reset(sampleRate, 0.03);
    hpQSm.reset(sampleRate, 0.03);
    lpSm.reset(sampleRate, 0.03);
    lpQSm.reset(sampleRate, 0.03);

    driveSm.setCurrentAndTargetValue(apvts.getRawParameterValue("drive")->load());
    mixSm.setCurrentAndTargetValue(apvts.getRawParameterValue("mix")->load());
    outSm.setCurrentAndTargetValue(apvts.getRawParameterValue("output")->load());
    symSm.setCurrentAndTargetValue(apvts.getRawParameterValue("symmetry")->load());
    rateSm.setCurrentAndTargetValue(apvts.getRawParameterValue("rate")->load());
    ceilSm.setCurrentAndTargetValue(apvts.getRawParameterValue("ceiling")->load());
    hpSm.setCurrentAndTargetValue(apvts.getRawParameterValue("hp")->load());
    hpQSm.setCurrentAndTargetValue(apvts.getRawParameterValue("hpq")->load());
    lpSm.setCurrentAndTargetValue(apvts.getRawParameterValue("lp")->load());
    lpQSm.setCurrentAndTargetValue(apvts.getRawParameterValue("lpq")->load());

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    for (int ch = 0; ch < 2; ++ch)
    {
        hpFilter[(size_t) ch].prepare(spec);
        lpFilter[(size_t) ch].prepare(spec);
        hpFilter[(size_t) ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        lpFilter[(size_t) ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        hpFilter[(size_t) ch].reset();
        lpFilter[(size_t) ch].reset();
    }

    dcX.fill(0.0f);
    dcY.fill(0.0f);
    envPeak.fill(0.0f);
    grainPhase.fill(0.0f);
    grainInc.fill(1.0f);
    grainAmp.fill(0.0f);
    lastSign.fill(1);
    halfCount.fill(0);
    lastHalf.fill(256);
    patternPhase.fill(0.0f);
}

void SampleDistortAudioProcessor::releaseResources()
{
    oversampling.reset();
}

bool SampleDistortAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != in)
        return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float SampleDistortAudioProcessor::readInterp(const std::vector<float>& data, float index)
{
    if (data.empty())
        return 0.0f;
    if (data.size() == 1)
        return data[0];

    index = juce::jlimit(0.0f, (float) data.size() - 1.001f, index);
    const int i0 = (int) index;
    const int i1 = juce::jmin(i0 + 1, (int) data.size() - 1);
    const float f = index - (float) i0;
    return data[(size_t) i0] + f * (data[(size_t) i1] - data[(size_t) i0]);
}

float SampleDistortAudioProcessor::processOne(float x, int channel, int mode, float drive,
                                               float symmetry, float rateHz, double osRate,
                                               const SampleData* data)
{
    const int ch = juce::jlimit(0, 1, channel);

    if (mode == 0)
    {
        const float driven = juce::jlimit(-1.0f, 1.0f, x * drive);
        const float idx = (driven * 0.5f + 0.5f) * (float) (kTableSize - 1);
        float asym = std::tanh(driven * 2.0f);
        float sym = asym;
        if (data != nullptr && !data->table.empty())
        {
            asym = readInterp(data->table, idx);
            sym = readInterp(data->symTable, idx);
        }
        return asym * (1.0f - symmetry) + sym * symmetry;
    }

    if (mode == 1)
    {
        const float ax = std::abs(x);
        envPeak[ch] = juce::jmax(envPeak[ch] * 0.9992f, ax);
        const int sign = x >= 0.0f ? 1 : -1;

        if (sign != lastSign[ch])
        {
            lastSign[ch] = sign;
            lastHalf[ch] = juce::jmax(8, halfCount[ch]);
            halfCount[ch] = 0;
            grainPhase[ch] = 0.0f;
            grainAmp[ch] = envPeak[ch] * juce::jlimit(0.25f, 8.0f, drive * 0.35f);
            if (data != nullptr && !data->mono.empty())
                grainInc[ch] = (float) data->mono.size() / (float) lastHalf[ch];
        }
        halfCount[ch]++;

        if (data == nullptr || data->mono.empty())
            return std::tanh(x * drive);

        float shaped = readInterp(data->mono, grainPhase[ch]) * grainAmp[ch];
        if ((shaped >= 0.0f) != (sign > 0))
            shaped = -shaped;
        grainPhase[ch] += grainInc[ch];
        if (grainPhase[ch] >= (float) data->mono.size() - 1.0f)
            grainPhase[ch] = (float) data->mono.size() - 1.0f;
        return shaped;
    }

    float mod = 0.0f;
    if (data != nullptr && !data->mono.empty())
    {
        const float len = (float) data->mono.size();
        mod = readInterp(data->mono, patternPhase[ch] * (len - 1.0f));
        patternPhase[ch] += (float) (rateHz / osRate);
        if (patternPhase[ch] >= 1.0f)
            patternPhase[ch] -= 1.0f;
    }

    const float amt = 0.35f + 2.4f * std::abs(mod);
    float y = std::sin(x * drive * amt * (1.0f + 3.0f * std::abs(mod)));
    return y * (0.85f + 0.15f * mod);
}

void SampleDistortAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(2, buffer.getNumChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer);

    auto* driveParam = apvts.getRawParameterValue("drive");
    auto* mixParam = apvts.getRawParameterValue("mix");
    auto* outParam = apvts.getRawParameterValue("output");
    auto* symParam = apvts.getRawParameterValue("symmetry");
    auto* rateParam = apvts.getRawParameterValue("rate");
    const int mode = (int) apvts.getRawParameterValue("mode")->load();
    const bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    driveSm.setTargetValue(driveParam->load());
    symSm.setTargetValue(symParam->load());
    rateSm.setTargetValue(rateParam->load());
    mixSm.setTargetValue(mixParam->load());
    outSm.setTargetValue(juce::Decibels::decibelsToGain(outParam->load()));
    ceilSm.setTargetValue(apvts.getRawParameterValue("ceiling")->load());
    hpSm.setTargetValue(apvts.getRawParameterValue("hp")->load());
    hpQSm.setTargetValue(apvts.getRawParameterValue("hpq")->load());
    lpSm.setTargetValue(apvts.getRawParameterValue("lp")->load());
    lpQSm.setTargetValue(apvts.getRawParameterValue("lpq")->load());

    if (!bypass)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto osBlock = oversampling.processSamplesUp(block);
        const int osN = (int) osBlock.getNumSamples();
        const double osRate = sampleRateHz * oversampling.getOversamplingFactor();
        const float dcR = std::exp(-2.0f * juce::MathConstants<float>::pi * 18.0f / (float) osRate);

        std::shared_ptr<SampleData> data;
        {
            juce::SpinLock::ScopedLockType sl(sampleLock);
            data = sampleData;
        }
        const SampleData* sample = data.get();

        for (int i = 0; i < osN; ++i)
        {
            const float drive = driveSm.getNextValue();
            const float symmetry = symSm.getNextValue();
            const float rate = rateSm.getNextValue();

            for (int ch = 0; ch < numCh; ++ch)
            {
                float x = osBlock.getSample(ch, i);
                float y = processOne(x, ch, mode, drive, symmetry, rate, osRate, sample);
                const float dc = y - dcX[(size_t) ch] + dcR * dcY[(size_t) ch];
                dcX[(size_t) ch] = y;
                dcY[(size_t) ch] = dc;
                osBlock.setSample(ch, i, dc);
            }
        }

        oversampling.processSamplesDown(block);
    }
    else
    {
        buffer.makeCopyOf(dry);
    }

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = bypass ? 0.0f : mixSm.getNextValue();
        const float out = outSm.getNextValue();
        const float ceiling = ceilSm.getNextValue();
        const float hpF = hpSm.getNextValue();
        const float hpQ = hpQSm.getNextValue();
        const float lpF = lpSm.getNextValue();
        const float lpQ = lpQSm.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float wet = buffer.getSample(ch, i);
            if (!bypass)
            {
                auto& hp = hpFilter[(size_t) ch];
                auto& lp = lpFilter[(size_t) ch];
                hp.setCutoffFrequency(hpF);
                hp.setResonance(hpQ);
                lp.setCutoffFrequency(lpF);
                lp.setResonance(lpQ);
                wet = hp.processSample(0, wet);
                wet = lp.processSample(0, wet);
                wet = std::tanh(wet / ceiling) * ceiling;
            }
            const float d = dry.getSample(ch, i);
            const float y = (d * (1.0f - mix) + wet * mix) * out;
            buffer.setSample(ch, i, y);
            peak = juce::jmax(peak, std::abs(y));
        }
    }
    meterDb.store(juce::Decibels::gainToDecibels(peak, -100.0f));
}

void SampleDistortAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("samplePath", lastFile.getFullPathName(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SampleDistortAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr)
        return;
    if (xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        auto path = apvts.state.getProperty("samplePath").toString();
        if (path.isNotEmpty())
        {
            juce::File f(path);
            if (f.existsAsFile())
                loadSampleFromFile(f);
        }
    }
}

juce::AudioProcessorEditor* SampleDistortAudioProcessor::createEditor()
{
    return new SampleDistortAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SampleDistortAudioProcessor();
}
