#include "guis/knulli/ExtendedGuiSettings.h"

#include "guis/GuiSettings.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "SystemConf.h"

static constexpr const char* DEFAULT_SWITCH_ON = "1";

ExtendedGuiSettings::ExtendedGuiSettings(Window* window, const char* title) : GuiSettings(window, _(title).c_str())
{
    // Do not nothing here, this is just a placeholder for the constructor
}

// Creates a new slider
std::shared_ptr<SliderComponent> ExtendedGuiSettings::createSlider(std::string label, float min, float max, float step, std::string unit, std::string description, bool show)
{
    std::shared_ptr<SliderComponent> slider = std::make_shared<SliderComponent>(mWindow, min, max, step, unit);
    if (!show) { // TODO: Awful hack to hide the slider, find a better way to do this
        return slider;
    }
    if (description.empty()) {
        addWithLabel(label, slider);
    } else {
        addWithDescription(label, description, slider);
    }
    return slider;
}

// Sets an initial value to a slider, either from default value or from variable if a batocera.conf variable for this slider has been set
void ExtendedGuiSettings::setConfigValueForSlider(std::shared_ptr<SliderComponent> slider, float defaultValue, std::string variable)
{
    float selectedValue = defaultValue;
    std::string configuredValue = SystemConf::getInstance()->get(variable);
    if (!configuredValue.empty()) {
        selectedValue = Utils::String::toFloat(configuredValue);
    }
    slider->setValue(selectedValue);
}

// Creates a new switch
std::shared_ptr<SwitchComponent> ExtendedGuiSettings::createSwitch(std::string label, std::string variable, std::string description, bool show)
{
    std::shared_ptr<SwitchComponent> switchComponent = std::make_shared<SwitchComponent>(mWindow);
    std::string selected = SystemConf::getInstance()->get(variable);
    if (selected.empty())
        selected = DEFAULT_SWITCH_ON;

    switchComponent->setState(selected == DEFAULT_SWITCH_ON);
    if (!show) { // TODO: Awful hack to hide the switch, find a better way to do this
        return switchComponent;
    }
    addWithDescription(label, description, switchComponent);
    return switchComponent;
}