#pragma once

#include <functional>
#include <string>

void installMacDropTarget(void* nativeView, std::function<void(const std::string&)> callback);
