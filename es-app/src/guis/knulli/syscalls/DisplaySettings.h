#pragma once
#include <string>
#include <vector>

class DisplaySettings
{
public:
        static bool hasDisplaySettings();
        static std::vector<std::string> getCapabilities();

};
