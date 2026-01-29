#pragma once

#include <functional>

#include "GuiComponent.h"
#include "Window.h"
#include "components/ImageGridComponent.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "components/TextComponent.h"
#include "components/ImageComponent.h"
#include "SaveState.h"

class ThemeData;
class FileData;

struct GameSwitcherItem
{
	GameSwitcherItem() : game(nullptr), saveState(nullptr) {}
	GameSwitcherItem(FileData* g, SaveState* s) : game(g), saveState(s) {}

	FileData* game;
	SaveState* saveState;
};

class GuiGameSwitcher : public GuiComponent
{
public:
	GuiGameSwitcher(Window* window);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void onSizeChanged() override;
	std::vector<HelpPrompt> getHelpPrompts() override;

	bool hitTest(int x, int y, Transform4x4f& parentTransform, std::vector<GuiComponent*>* pResult = nullptr) override;
	bool onMouseClick(int button, bool pressed, int x, int y);

	static bool hasRecentGames();

private:
	void centerWindow();
	void loadRecentGames();
	void updateSelectedInfo();
	void launchGame();
	std::string formatPlayTime(int seconds);

	std::shared_ptr<ImageGridComponent<GameSwitcherItem>> mGrid;
	std::shared_ptr<ThemeData> mTheme;
	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mGameName;
	std::shared_ptr<TextComponent> mPlayTime;
	std::shared_ptr<ImageComponent> mWheelImage;

	NinePatchComponent mBackground;
	ComponentGrid mLayout;
	std::vector<GameSwitcherItem> mItems;
};
