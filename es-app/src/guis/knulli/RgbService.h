#pragma once
#include <string>
#include <vector>

struct ModeInfo
{
  std::string id;
  std::string name;
};

struct PaletteInfo
{
  std::string id;
  std::string name;
};

class RgbService
{
public:
        static void reloadConfig();
        static std::vector<std::string> requiredSettings();
        static std::vector<ModeInfo> getAvailableModes();
        static std::vector<PaletteInfo> getAvailablePalettes();
        static void applyValue(std::string key, std::string value);

};
