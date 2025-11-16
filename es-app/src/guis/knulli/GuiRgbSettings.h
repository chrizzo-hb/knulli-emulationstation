#pragma once
#include "ExtendedGuiSettings.h"
#include "components/SliderComponent.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include <array>
#include <memory>

class GuiRgbSettings : public ExtendedGuiSettings
{
public:
    GuiRgbSettings(Window* window);

private:

    bool isH700;
    bool isA133;
    
    std::shared_ptr<OptionListComponent<std::string>> createModeOptionList();
    std::shared_ptr<OptionListComponent<std::string>> createPaletteOptionList(const std::string& configKey, const std::string& title, const std::string& description);
    std::shared_ptr<OptionListComponent<std::string>> createBatteryIndicationOptionList(const std::string& configKey, const std::string& title, const std::string& description);

    std::shared_ptr<OptionListComponent<std::string>> optionListMode;
    std::shared_ptr<OptionListComponent<std::string>> optionListPalettePrimary;
    std::shared_ptr<OptionListComponent<std::string>> optionListPaletteSecondary;
    std::shared_ptr<OptionListComponent<std::string>> optionListBatteryCharging;
    std::shared_ptr<OptionListComponent<std::string>> optionListBatteryLow;

    std::shared_ptr<SliderComponent> sliderLedBrightness;
    std::shared_ptr<SliderComponent> sliderLedSpeed;
    std::shared_ptr<SwitchComponent> switchAdaptiveBrightness;
    std::shared_ptr<SwitchComponent> switchPaletteSwap;
    std::shared_ptr<SliderComponent> sliderLowBatteryThreshold;
    std::shared_ptr<SwitchComponent> switchBatteryCharging;
    std::shared_ptr<SwitchComponent> switchRetroAchievements;

};