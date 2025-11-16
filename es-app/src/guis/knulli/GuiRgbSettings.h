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

};