#pragma once

#include "guis/GuiSettings.h"

class Window;

class GuiDiskCheck : public GuiSettings
{
public:
	GuiDiskCheck(Window* window);
    void pressedStart();
    
};