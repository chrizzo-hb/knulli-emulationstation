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

 
}
