#include "guis/knulli/GuiRgbSettings.h"
#include "guis/GuiMsgBox.h"
#include "guis/knulli/ExtendedGuiSettings.h"
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "Scripting.h"
#include "InputManager.h"
#include "AudioManager.h"
#include <SDL_events.h>
#include <algorithm>
#include <memory>
#include <string>
#include "RgbService.h"
#include "BoardCheck.h"

#include "Log.h"

constexpr const char* DEFAULT_LED_MODE = "static";
constexpr const char* DEFAULT_LED_PALETTE = "Knulli";
constexpr const char* DEFAULT_BATTERY_MODE = "continuous";
constexpr float DEFAULT_LOW_BATTERY_THRESHOLD = 20;
constexpr float DEFAULT_BRIGHTNESS = 100;

constexpr const char* MENU_EVENT_NAME = "rgb-changed";

// Constructor creates a new GuiRgbSettings menu.
GuiRgbSettings::GuiRgbSettings(Window* window) : ExtendedGuiSettings(window, "RGB LED SETTINGS")
{
    requiredSettings = RgbService::requiredSettings();

    if (requiredSettings.empty()) {
        LOG(LogWarning) << "No required RGB settings available from RgbService, RGB settings menu will be empty.";
    }
    if ((hasRequiredSetting("mode") == true || hasRequiredSetting("palette") == true)
            || hasRequiredSetting("palette.invert") || hasRequiredSetting("palette.invert.secondary")
            || hasRequiredSetting("brightness") == true || hasRequiredSetting("brightness.adaptive") == true)
    {
        addGroup(_("REGULAR LED MODE AND COLOR"));
    }

    // LED Mode Options
    optionListMode = createModeOptionList();
    optionListMode->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("mode", value); });

    optionListPalettePrimary = createPaletteOptionList("palette", "COLOR PALETTE", "Select the color palette.");
    optionListPalettePrimary->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("palette", value); });
    
    // Swap colors switch
    switchPaletteInvert = createSwitch(_("INVERT COLORS"), "led.palette.invert", _("Inverts primary and secondary color of the color palette."), false, false, hasRequiredSetting("palette.invert"));
    switchPaletteInvert->setOnChangedCallback([this]() { RgbService::applyValue("palette.invert", switchPaletteInvert->getState() ? "1" : "0"); });

    // Swap colors switch
    switchPaletteInvertSecondary = createSwitch(_("INVERT COLORS (SECONDARY)"), "led.palette.invert.secondary", _("Inverts primary and secondary color of the color palette on secondary LEDs."), false, false, hasRequiredSetting("palette.invert.secondary"));
    switchPaletteInvertSecondary->setOnChangedCallback([this]() { RgbService::applyValue("palette.invert.secondary", switchPaletteInvertSecondary->getState() ? "1" : "0"); });

    // Palette modification options
    optionListPaletteMod = createPaletteModOptionList();
    optionListPaletteMod->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("palette.mod", value); }); 

    // LED Brightness Slider
    sliderLedBrightness = createSlider(_("BRIGHTNESS"), 0.f, 10.f, 1.f, "", "", hasRequiredSetting("brightness"));
    setConfigValueForSlider(sliderLedBrightness, DEFAULT_BRIGHTNESS, "led.brightness");
    sliderLedBrightness->setOnValueChanged([this](float value) { RgbService::applyValue("brightness", std::to_string((int)value)); });

    // Adaptive Brightness switch
    switchAdaptiveBrightness = createSwitch(_("ADAPTIVE BRIGHTNESS"), "led.brightness.adaptive", _("Automatically adapts LED brightness to screen brightness (based on the brightness setting above)."), true, false, hasRequiredSetting("brightness.adaptive"));
    switchAdaptiveBrightness->setOnChangedCallback([this]() { RgbService::applyValue("brightness.adaptive", switchAdaptiveBrightness->getState() ? "1" : "0"); });

    if (hasRequiredSetting("battery.low") == true || hasRequiredSetting("battery.charging") == true || hasRequiredSetting("battery.low.threshold") == true)
        addGroup(_("BATTERY CHARGE INDICATION"));

    // Low battery threshold slider
    sliderLowBatteryThreshold = createSlider(_("LOW BATTERY THRESHOLD"), 0.f, 30.f, 1.f, "%", _("Threshold for low battery indication."), hasRequiredSetting("battery.low.threshold"));
    setConfigValueForSlider(sliderLowBatteryThreshold, DEFAULT_LOW_BATTERY_THRESHOLD, "led.battery.low.threshold");
    sliderLowBatteryThreshold->setOnValueChanged([this](float value) { RgbService::applyValue("battery.low.threshold", std::to_string((int)value)); });
    optionListBatteryLow = createBatteryIndicationOptionList("battery.low", "LOW BATTERY INDICATION", "Select the type of low battery indication.");
    optionListBatteryLow->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("battery.low", value); });
    optionListBatteryCharging = createBatteryIndicationOptionList("battery.charging", "BATTERY CHARGING INDICATION", "Select the type of battery charging indication.");
    optionListBatteryCharging->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("battery.charging", value); });


    if (hasRequiredSetting("retroachievements") == true)
        addGroup(_("RETRO ACHIEVEMENT INDICATION"));
    switchRetroAchievements = createSwitch(_("ACHIEVEMENT EFFECT"), "led.retroachievements", _("Honor your retro achievements with a LED effect."), true, false, hasRequiredSetting("retroachievements"));

    addSaveFunc([this] {
        // Read all variables from the respective UI elements and set the respective values in batocera.conf
        SystemConf::getInstance()->set("led.mode", optionListMode->getSelected());
        SystemConf::getInstance()->set("led.palette", optionListPalettePrimary->getSelected());
        SystemConf::getInstance()->set("led.brightness", std::to_string((int) sliderLedBrightness->getValue()));
        SystemConf::getInstance()->set("led.brightness.adaptive", (switchAdaptiveBrightness->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.battery.low.threshold", std::to_string((int) sliderLowBatteryThreshold->getValue()));
        SystemConf::getInstance()->set("led.battery.low", optionListBatteryLow->getSelected());
        SystemConf::getInstance()->set("led.battery.charging", optionListBatteryCharging->getSelected());
        SystemConf::getInstance()->set("led.retroachievements", (switchRetroAchievements->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.palette.invert", (switchPaletteInvert->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.palette.invert.secondary", (switchPaletteInvertSecondary->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.palette.mod", optionListPaletteMod->getSelected());
		SystemConf::getInstance()->saveSystemConf();
		Scripting::fireEvent(MENU_EVENT_NAME);

        // Force reloading settings for the RGB service
        RgbService::reloadConfig();
    });

}

// Creates a new mode option list
std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createModeOptionList()
{
    auto optionsLedMode = std::make_shared<OptionListComponent<std::string>>(mWindow, _("MODE"), false);

    std::string selectedLedMode = "null";
    std::vector<ModeInfo> availableModes = RgbService::getAvailableModes();

    if (availableModes.empty()) {
        LOG(LogWarning) << "No RGB modes available from RgbService, adding default options.";
        optionsLedMode->add(_("None"), "null", selectedLedMode == "null");
    }
    else
    {   
        std::string configuredLedMode = SystemConf::getInstance()->get("led.mode");
        if (configuredLedMode.empty()) {
            configuredLedMode = DEFAULT_LED_MODE;
        }

        selectedLedMode = DEFAULT_LED_MODE;
        for (const auto& mode : availableModes) {
            if (configuredLedMode == mode.id) {
                selectedLedMode = configuredLedMode;
                optionsLedMode->add(_(mode.name.c_str()), mode.id, true);
            } else {
                optionsLedMode->add(_(mode.name.c_str()), mode.id, false);
            }
            
        }
    }

    if (hasRequiredSetting("mode") == true)
        addWithDescription(_("MODE"), _("Set the default LED animation"), optionsLedMode);
    return optionsLedMode;
}

std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createPaletteOptionList(const std::string& setting, const std::string& title, const std::string& description)
{
    auto optionsLedPalette = std::make_shared<OptionListComponent<std::string>>(mWindow, _(title.c_str()), false);

    std::string configKey = "led." + setting;
    std::string selectedLedPalette = SystemConf::getInstance()->get(configKey);
    std::vector<PaletteInfo> availablePalettes = RgbService::getAvailablePalettes();
    if (selectedLedPalette.empty() && !availablePalettes.empty())
        selectedLedPalette = DEFAULT_LED_PALETTE;

    for (const auto& palette : availablePalettes) {
        // XXX: Shorten the palette name by removing the color specification in parentheses
        // e.g., "Cotton Candy (Pink, Sky Blue)" becomes "Cotton Candy", just because the
        // overly long names cause misalignment in the GUI.
        std::string shortenedName = palette.name.substr(0, palette.name.find(" ("));
        optionsLedPalette->add(shortenedName, palette.id, selectedLedPalette == palette.id);
    }

    if (hasRequiredSetting(setting) == true)
        addWithDescription(_(title.c_str()), _(description.c_str()), optionsLedPalette);
    return optionsLedPalette;
}

std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createBatteryIndicationOptionList(const std::string& setting, const std::string& title, const std::string& description)
{
    auto optionsBatteryIndication = std::make_shared<OptionListComponent<std::string>>(mWindow, _(title.c_str()), false);

    std::string configKey = "led." + setting;
    std::string selectedOption = SystemConf::getInstance()->get(configKey);
    if (selectedOption.empty() || (selectedOption != "off" && selectedOption != "notification" && selectedOption != "continuous"))
        selectedOption = DEFAULT_BATTERY_MODE;

    optionsBatteryIndication->add(_("None"), "off", selectedOption == "off");
    optionsBatteryIndication->add(_("Notification"), "notification", selectedOption == "notification");
    optionsBatteryIndication->add(_("Continuous"), "continuous", selectedOption == "continuous");

    if (hasRequiredSetting(setting) == true)
        addWithDescription(_(title.c_str()), _(description.c_str()), optionsBatteryIndication);
    return optionsBatteryIndication;
}

std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createPaletteModOptionList()
{
    auto optionsPaletteMod = std::make_shared<OptionListComponent<std::string>>(mWindow, _("PALETTE MODIFICATION"), false);

    std::string selectedPaletteMod = SystemConf::getInstance()->get("led.palette.mod");
    if (selectedPaletteMod.empty())
        selectedPaletteMod = "none";

    optionsPaletteMod->add(_("None"), "none", selectedPaletteMod == "none");
    optionsPaletteMod->add(_("Twilight"), "twilight", selectedPaletteMod == "twilight");
    optionsPaletteMod->add(_("Sparkle"), "sparkle", selectedPaletteMod == "sparkle");
    optionsPaletteMod->add(_("Haze"), "haze", selectedPaletteMod == "haze");

    if (hasRequiredSetting("palette.mod") == true)
        addWithDescription(_("PALETTE MOD"), _("Select a modification effect for the color palette."), optionsPaletteMod);
    return optionsPaletteMod;
}

void GuiRgbSettings::applyValue(const std::string& key, const std::string& value)
{
    LOG(LogError) << "GuiRgbSettings::applyValue called with key: " << key << " value: " << value;
    RgbService::applyValue(key, value);
}

bool GuiRgbSettings::hasRequiredSetting(std::string setting)
{
    return std::find(requiredSettings.begin(), requiredSettings.end(), setting) != requiredSettings.end();
}
