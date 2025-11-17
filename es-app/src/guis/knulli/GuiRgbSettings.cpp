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

const std::vector<std::string> RGB_BOARDS_H700 = {"rg40xx-h", "rg40xx-v", "rg-cubexx"};
const std::vector<std::string> RGB_BOARDS_A133 = {"trimui-smart-pro", "trimui-brick"};

// Still required
constexpr const char* DEFAULT_LED_MODE = "static";
constexpr const char* DEFAULT_LED_PALETTE = "Knulli";
constexpr float DEFAULT_LOW_BATTERY_THRESHOLD = 20;
constexpr float DEFAULT_BRIGHTNESS = 100;

constexpr const char* MENU_EVENT_NAME = "rgb-changed";

// Constructor creates a new GuiRgbSettings menu.
GuiRgbSettings::GuiRgbSettings(Window* window) : ExtendedGuiSettings(window, "RGB LED SETTINGS")
{
    // TODO: This should not be hard-coded, it should be read from a file or a service.
    isH700 = BoardCheck::isBoard(RGB_BOARDS_H700);
    isA133 = BoardCheck::isBoard(RGB_BOARDS_A133);

    addGroup(_("REGULAR LED MODE AND COLOR"));

    // LED Mode Options
    optionListMode = createModeOptionList();
    optionListMode->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("mode", value); });

    optionListPalettePrimary = createPaletteOptionList("led.palette", "PRIMARY PALETTE", "Select the main color palette.");
    optionListPalettePrimary->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("palette", value); });
    optionListPaletteSecondary = createPaletteOptionList("led.palette.secondary", "SECONDARY PALETTE", "Select the secondary color palette.");
    optionListPaletteSecondary->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("palette.secondary", value); });

    // Adaptive Brightness switch
    switchPaletteSwap = createSwitch(_("SWAP PALETTES"), "led.palette.swap", _("Swaps main and secondary color palettes."), false, false, (isH700 || isA133));
    switchPaletteSwap->setOnChangedCallback([this]() { RgbService::applyValue("palette.swap", switchPaletteSwap->getState() ? "true" : "false"); });

    // LED Brightness Slider
    sliderLedBrightness = createSlider(_("BRIGHTNESS"), 0.f, 10.f, 1.f, "", "", (isH700 || isA133));
    setConfigValueForSlider(sliderLedBrightness, DEFAULT_BRIGHTNESS, "led.brightness");
    sliderLedBrightness->setOnValueChanged([this](float value) { RgbService::applyValue("brightness", std::to_string((int)value)); });

    // Adaptive Brightness switch
    switchAdaptiveBrightness = createSwitch(_("ADAPTIVE BRIGHTNESS"), "led.brightness.adaptive", _("Automatically adapts LED brightness to screen brightness (based on the brightness setting above)."), true, false, (isH700 || isA133));
    switchAdaptiveBrightness->setOnChangedCallback([this]() { RgbService::applyValue("brightness.adaptive", switchAdaptiveBrightness->getState() ? "true" : "false"); });

    addGroup(_("BATTERY CHARGE INDICATION"));

    // Low battery threshold slider
    sliderLowBatteryThreshold = createSlider(_("LOW BATTERY THRESHOLD"), 0.f, 30.f, 1.f, "%", _("Threshold for low battery indication."), (isH700 || isA133));
    setConfigValueForSlider(sliderLowBatteryThreshold, DEFAULT_LOW_BATTERY_THRESHOLD, "led.battery.low.threshold");
    sliderLowBatteryThreshold->setOnValueChanged([this](float value) { RgbService::applyValue("battery.low.threshold", std::to_string((int)value)); });
    optionListBatteryLow = createBatteryIndicationOptionList("led.battery.low", "LOW BATTERY INDICATION", "Select the type of low battery indication.");
    optionListBatteryLow->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("battery.low", value); });
    optionListBatteryCharging = createBatteryIndicationOptionList("led.battery.charging", "BATTERY CHARGING INDICATION", "Select the type of battery charging indication.");
    optionListBatteryCharging->setSelectedChangedCallback([this](std::string value) { RgbService::applyValue("battery.charging", value); });


    addGroup(_("RETRO ACHIEVEMENT INDICATION"));
    switchRetroAchievements = createSwitch(_("ACHIEVEMENT EFFECT"), "led.retroachievements", _("Honor your retro achievements with a LED effect."), true, false, (isH700 || isA133));

    addSaveFunc([this] {
        // Read all variables from the respective UI elements and set the respective values in batocera.conf
        SystemConf::getInstance()->set("led.mode", optionListMode->getSelected());
        SystemConf::getInstance()->set("led.palette", optionListPalettePrimary->getSelected());
        SystemConf::getInstance()->set("led.palette.secondary", optionListPaletteSecondary->getSelected());
        SystemConf::getInstance()->set("led.brightness", std::to_string((int) sliderLedBrightness->getValue()));
        SystemConf::getInstance()->set("led.brightness.adaptive", (switchAdaptiveBrightness->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.battery.low.threshold", std::to_string((int) sliderLowBatteryThreshold->getValue()));
        SystemConf::getInstance()->set("led.battery.low", optionListBatteryLow->getSelected());
        SystemConf::getInstance()->set("led.battery.charging", optionListBatteryCharging->getSelected());
        SystemConf::getInstance()->set("led.retroachievements", (switchRetroAchievements->getState() ? "1" : "0"));
        SystemConf::getInstance()->set("led.palette.swap", (switchPaletteSwap->getState() ? "1" : "0"));
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

    std::string selectedLedMode = SystemConf::getInstance()->get("led.mode");
    std::vector<ModeInfo> availableModes = RgbService::getAvailableModes();
    if (selectedLedMode.empty())
        selectedLedMode = DEFAULT_LED_MODE;

    if (availableModes.empty()) {
        LOG(LogWarning) << "No RGB modes available from RgbService, adding default options.";
        optionsLedMode->add("None", "null", selectedLedMode == "null");
    }
    else
    {
        for (const auto& mode : availableModes) {
            optionsLedMode->add(mode.name, mode.id, selectedLedMode == mode.id);
        }
    }

    addWithDescription(_("MODE"), _("Set the default LED animation"), optionsLedMode);
    return optionsLedMode;
}

std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createPaletteOptionList(const std::string& configKey, const std::string& title, const std::string& description)
{
    auto optionsLedPalette = std::make_shared<OptionListComponent<std::string>>(mWindow, _(title.c_str()), false);

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

    addWithDescription(_(title.c_str()), _(description.c_str()), optionsLedPalette);
    return optionsLedPalette;
}

std::shared_ptr<OptionListComponent<std::string>> GuiRgbSettings::createBatteryIndicationOptionList(const std::string& configKey, const std::string& title, const std::string& description)
{
    auto optionsBatteryIndication = std::make_shared<OptionListComponent<std::string>>(mWindow, _(title.c_str()), false);

    std::string selectedOption = SystemConf::getInstance()->get(configKey);
    if (selectedOption.empty())
        selectedOption = "off";

    optionsBatteryIndication->add("None", "off", selectedOption == "off");
    optionsBatteryIndication->add("Notification", "notification", selectedOption == "notification");
    optionsBatteryIndication->add("Continuous", "continuous", selectedOption == "continuous");

    addWithDescription(_(title.c_str()), _(description.c_str()), optionsBatteryIndication);
    return optionsBatteryIndication;
}

void GuiRgbSettings::applyValue(const std::string& key, const std::string& value)
{
    LOG(LogError) << "GuiRgbSettings::applyValue called with key: " << key << " value: " << value;
    RgbService::applyValue(key, value);
}