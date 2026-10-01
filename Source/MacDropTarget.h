#pragma once

#include <JuceHeader.h>
#include <functional>

void installMacDropTarget(void* nativeView, std::function<void(const juce::String&)> callback);
