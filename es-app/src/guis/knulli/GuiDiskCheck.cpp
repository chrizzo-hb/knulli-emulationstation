#include "GuiDiskCheck.h"

#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "guis/GuiMsgBox.h"
#include "ThreadedDiskCheck.h"

GuiDiskCheck::GuiDiskCheck(Window* window) : GuiSettings(window, _("DISK CHECK").c_str())
{

	mMenu.clearButtons();
	mMenu.addButton(_("RUN DISK CHECK"), _("START"), std::bind(&GuiDiskCheck::pressedStart, this));
	mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });

}

void GuiDiskCheck::pressedStart()
{
	if (ThreadedDiskCheck::isRunning())
		mWindow->pushGui(new GuiMsgBox(mWindow, _("DISK CHECK IS ALREADY RUNNING.")));
	else
    {
	    ThreadedDiskCheck::start(mWindow);
        close();
    }
}