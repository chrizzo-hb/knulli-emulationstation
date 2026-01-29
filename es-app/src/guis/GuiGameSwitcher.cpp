#include "GuiGameSwitcher.h"
#include "SystemData.h"
#include "FileData.h"
#include "utils/StringUtil.h"
#include "utils/FileSystemUtil.h"
#include "HelpStyle.h"
#include "guis/GuiMsgBox.h"
#include "SaveStateRepository.h"
#include "CollectionSystemManager.h"
#include "views/ViewController.h"
#include "Settings.h"
#include "LocaleES.h"

#define WINDOW_HEIGHT Renderer::getScreenHeight() * 0.50f

static int slots = 5;

GuiGameSwitcher::GuiGameSwitcher(Window* window) :
	GuiComponent(window), mGrid(nullptr), mBackground(window, ":/frame.png"),
	mLayout(window, Vector2i(3, 7)), mTitle(nullptr), mGameName(nullptr),
	mPlayTime(nullptr), mWheelImage(nullptr)
{
	// Form background
	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);
	mBackground.setPostProcessShader(theme->Background.menuShader);

	mTitle = std::make_shared<TextComponent>(mWindow, _("GAME SWITCHER"), theme->Title.font, theme->Title.color, ALIGN_CENTER);
	mLayout.setEntry(mTitle, Vector2i(1, 1), false, true);

	mGameName = std::make_shared<TextComponent>(mWindow, "", theme->Text.font, theme->Text.color, ALIGN_CENTER);
	mLayout.setEntry(mGameName, Vector2i(1, 2), false, true);

	mPlayTime = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, ALIGN_CENTER);
	mLayout.setEntry(mPlayTime, Vector2i(1, 3), false, true);

	mGrid = std::make_shared<ImageGridComponent<GameSwitcherItem>>(mWindow);
	mLayout.setEntry(mGrid, Vector2i(1, 5), true, true);

	addChild(&mBackground);
	addChild(&mLayout);

	float cellProportion = 1.77;
	float screenProportion = (float)Renderer::getScreenWidth() / (float)Renderer::getScreenHeight();

	float sh = (float)Math::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
	sh = (float)theme->TextSmall.font->getSize() / sh;

	std::string xml =
		"<theme defaultView=\"Tiles\">"
		"<formatVersion>7</formatVersion>"
		"<view name = \"grid\">"
		"<imagegrid name=\"gamegrid\">"
		"  <margin>0.01 0.02</margin>"
		"  <padding>0 0</padding>"
		"  <scrollDirection>horizontal</scrollDirection>"
		"  <autoLayout>" + std::to_string(slots * screenProportion / cellProportion) + " 1</autoLayout>"
		"  <autoLayoutSelectedZoom>1</autoLayoutSelectedZoom>"
		"  <animateSelection>false</animateSelection>"
		"  <centerSelection>true</centerSelection>"
		"</imagegrid>"
		"<gridtile name=\"default\">"
		"  <backgroundColor>FFFFFF00</backgroundColor>"
		"  <padding>8 8</padding>"
		"  <imageColor>FFFFFFFF</imageColor>"
		"</gridtile>"
		"<gridtile name=\"selected\">"
		"  <backgroundColor>" + Utils::String::toHexString(theme->Text.selectorColor) + "</backgroundColor>"
		"</gridtile>"
		"<text name=\"gridtile\">"
		"  <color>" + Utils::String::toHexString(theme->Text.color) + "</color>"
		"  <backgroundColor>00000000</backgroundColor>"
		"  <fontPath>" + theme->Text.font->getPath() + "</fontPath>"
		"  <fontSize>" + std::to_string(sh) + "</fontSize>"
		"  <alignment>center</alignment>"
		"  <singleLineScroll>false</singleLineScroll>"
		"  <size>1 0.30</size>"
		"</text>"
		"<text name=\"gridtile:selected\">"
		"  <color>" + Utils::String::toHexString(theme->Text.selectedColor) + "</color>"
		"</text>"
		"<image name=\"gridtile.image\">"
		"  <linearSmooth>true</linearSmooth>"
		"  <color>D0D0D0D0</color>"
		"  <roundCorners>0.02</roundCorners>"
		"</image>"
		"<image name=\"gridtile.image:selected\">"
		"  <color>FFFFFFFF</color>"
		"</image>"
		"</view>"
		"</theme>";

	mTheme = std::shared_ptr<ThemeData>(new ThemeData());
	std::map<std::string, std::string> emptyMap;
	mTheme->loadFile("gameswitcher", emptyMap, xml, false);

	mGrid->applyTheme(mTheme, "grid", "gamegrid", 0);
	mGrid->setCursorChangedCallback([&](const CursorState& /*state*/) {
		updateSelectedInfo();
		updateHelpPrompts();
	});

	loadRecentGames();
	centerWindow();
}

void GuiGameSwitcher::loadRecentGames()
{
	mGrid->clear();
	mGrid->onSizeChanged();
	mItems.clear();

	int maxCount = Settings::getInstance()->getInt("GameSwitcherCount");
	if (maxCount <= 0)
		maxCount = 10;

	auto& autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();
	auto it = autoSystems.find("recent");
	if (it == autoSystems.end() || it->second.system == nullptr)
		return;

	auto recentSystem = it->second.system;
	auto rootFolder = recentSystem->getRootFolder();
	if (rootFolder == nullptr)
		return;

	auto games = rootFolder->getChildrenListToDisplay();
	int count = 0;

	for (auto fileData : games)
	{
		if (count >= maxCount)
			break;

		if (fileData->getType() != GAME)
			continue;

		FileData* game = fileData->getSourceFileData();
		if (game == nullptr)
			game = fileData;

		// Check if the game file still exists
		if (!Utils::FileSystem::exists(game->getPath()))
			continue;

		// Get autosave for this game if available
		SaveState* saveState = nullptr;
		auto system = game->getSystem();
		if (system != nullptr)
		{
			auto repo = system->getSaveStateRepository();
			if (repo != nullptr)
				saveState = repo->getGameAutoSave(game);
		}

		// Determine image to show: save state screenshot > game image > thumbnail
		std::string imagePath;
		if (saveState != nullptr && !saveState->getScreenShot().empty())
			imagePath = saveState->getScreenShot();
		else if (!game->getImagePath().empty())
			imagePath = game->getImagePath();
		else if (!game->getThumbnailPath().empty())
			imagePath = game->getThumbnailPath();

		// Use a default image if no image found
		if (imagePath.empty())
			imagePath = ":/cartridge.svg";

		mItems.push_back(GameSwitcherItem(game, saveState));
		mGrid->add(game->getName(), imagePath, mItems.back());
		count++;
	}

	if (mGrid->size() > 0)
		updateSelectedInfo();
}

void GuiGameSwitcher::updateSelectedInfo()
{
	if (mGrid->size() == 0)
	{
		mGameName->setText("");
		mPlayTime->setText("");
		return;
	}

	const GameSwitcherItem& item = mGrid->getSelected();
	if (item.game == nullptr)
		return;

	mGameName->setText(item.game->getName());

	int gameTime = item.game->getMetadata().getInt(MetaDataId::GameTime);
	if (gameTime > 0)
	{
		std::string playTimeStr = formatPlayTime(gameTime);
		if (item.saveState != nullptr)
			playTimeStr += " - " + _("SAVE STATE AVAILABLE");
		mPlayTime->setText(playTimeStr);
	}
	else if (item.saveState != nullptr)
	{
		mPlayTime->setText(_("SAVE STATE AVAILABLE"));
	}
	else
	{
		mPlayTime->setText("");
	}
}

std::string GuiGameSwitcher::formatPlayTime(int seconds)
{
	if (seconds < 60)
		return std::to_string(seconds) + " " + _("seconds");

	int minutes = seconds / 60;
	if (minutes < 60)
		return std::to_string(minutes) + " " + _("minutes");

	int hours = minutes / 60;
	minutes = minutes % 60;

	if (hours < 24)
	{
		if (minutes > 0)
			return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
		return std::to_string(hours) + " " + _("hours");
	}

	int days = hours / 24;
	hours = hours % 24;

	if (hours > 0)
		return std::to_string(days) + "d " + std::to_string(hours) + "h";
	return std::to_string(days) + " " + _("days");
}

void GuiGameSwitcher::launchGame()
{
	if (mGrid->size() == 0)
		return;

	const GameSwitcherItem& item = mGrid->getSelected();
	if (item.game == nullptr)
		return;

	// Check if game file still exists
	if (!Utils::FileSystem::exists(item.game->getPath()))
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _("GAME FILE NOT FOUND"), _("OK")));
		return;
	}

	LaunchGameOptions options;
	options.saveStateInfo = item.saveState; // May be nullptr for normal launch

	ViewController::get()->launch(item.game, options);
}

void GuiGameSwitcher::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	float helpSize = 0.02;

	if (Settings::getInstance()->getBool("ShowHelpPrompts"))
	{
		HelpStyle help;
		help.applyTheme(mTheme, "system");

		const float height = Math::round(help.font->getLetterHeight() * 1.25f);

		float helpBottom = help.position.y() + (height * mOrigin.y());
		float helpBottomSpace = 0;

		float helpTop = help.position.y() - (height * mOrigin.y());

		helpSize = helpTop;
		helpSize = Renderer::getScreenHeight() - helpSize + helpBottomSpace;
		helpSize = helpSize / mSize.y() + 0.06;
	}

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mLayout.setColWidthPerc(0, 0.01);
	mLayout.setColWidthPerc(2, 0.01);

	mLayout.setRowHeightPerc(0, 0.02);

	if (mTitle != nullptr && mTitle->getFont() != nullptr)
		mLayout.setRowHeightPerc(1, mTitle->getFont()->getHeight(2.0f) / mSize.y());

	// Game name row
	if (mGameName != nullptr && mGameName->getFont() != nullptr)
		mLayout.setRowHeightPerc(2, mGameName->getFont()->getHeight(1.5f) / mSize.y());

	// Play time row
	if (mPlayTime != nullptr && mPlayTime->getFont() != nullptr)
		mLayout.setRowHeightPerc(3, mPlayTime->getFont()->getHeight(1.5f) / mSize.y());

	mLayout.setRowHeightPerc(4, 0.02);
	mLayout.setRowHeightPerc(6, helpSize);

	mLayout.setSize(mSize);
}

void GuiGameSwitcher::centerWindow()
{
	setSize(Renderer::getScreenWidth(), WINDOW_HEIGHT);
	animateTo(
		Vector2f((Renderer::getScreenWidth() - getSize().x()) / 2, Renderer::getScreenHeight()),
		Vector2f((Renderer::getScreenWidth() - getSize().x()) / 2, Renderer::getScreenHeight() - WINDOW_HEIGHT),
		AnimateFlags::OPACITY | AnimateFlags::POSITION);
}

bool GuiGameSwitcher::input(InputConfig* config, Input input)
{
	if (input.value != 0 && (config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("l3", input)))
	{
		delete this;
		return true;
	}

	if (input.value != 0 && config->isMappedTo(BUTTON_OK, input))
	{
		launchGame();
		delete this;
		return true;
	}

	return GuiComponent::input(config, input);
}

void GuiGameSwitcher::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
}

std::vector<HelpPrompt> GuiGameSwitcher::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK"), [&] { delete this; }));

	if (mGrid->size() > 0)
	{
		const GameSwitcherItem& item = mGrid->getSelected();
		if (item.saveState != nullptr)
			prompts.push_back(HelpPrompt(BUTTON_OK, _("RESUME")));
		else
			prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	}

	return prompts;
}

bool GuiGameSwitcher::hitTest(int x, int y, Transform4x4f& parentTransform, std::vector<GuiComponent*>* pResult)
{
	if (pResult) pResult->push_back(this);
	GuiComponent::hitTest(x, y, parentTransform, pResult);
	return true;
}

bool GuiGameSwitcher::onMouseClick(int button, bool pressed, int x, int y)
{
	if (pressed && button == 1 && !mBackground.isMouseOver())
	{
		delete this;
		return true;
	}

	return (button == 1);
}

bool GuiGameSwitcher::hasRecentGames()
{
	auto& autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();
	auto it = autoSystems.find("recent");
	if (it == autoSystems.end() || it->second.system == nullptr)
		return false;

	auto rootFolder = it->second.system->getRootFolder();
	if (rootFolder == nullptr)
		return false;

	auto games = rootFolder->getChildrenListToDisplay();
	return !games.empty();
}
