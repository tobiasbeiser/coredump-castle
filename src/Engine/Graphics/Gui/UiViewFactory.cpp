
#include "UiViewFactory.hpp"

#include "Engine/GameEngine.hpp"
#include "UiBuilder.hpp"

#include <utility>

#include "Engine/Graphics/TextureManager.hpp"
#include "Engine/Utils/SaveGameFormatter.hpp"
#include "Layouts/Container.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/Frame.hpp"
#include "Widgets/Image.hpp"
#include "Widgets/Label.hpp"

namespace LayoutConstants {
constexpr float headerHeight = 80.0f;
constexpr float footerHeight = 60.0f;
constexpr float menuGap = 30.0f;
constexpr float buttonMenuWidth = 200.0f;
constexpr float panelContentWidth = 300.0f;
constexpr float gap = 8.0f;
constexpr float saveRowHeight = 60.0f;
constexpr float settingsRowHeight = 50.0f;
} // namespace LayoutConstants

namespace {
SDL_FRect getScreenRect(const Ui::UIContext &ctx)
{
	const auto [width, height] = ctx.renderer->getLogicalResolution();
	return {0, 0, static_cast<float>(width), static_cast<float>(height)};
}

std::unique_ptr<Ui::Widget> createStandardScreen(const Ui::UIContext &ctx, const std::string_view titleText,
                                                 Ui::UiBuilder<Ui::Container> &&contentBuilder, float contentWidth,
                                                 const std::string_view bottomButtonText = "",
                                                 std::function<void()> onBottomButton = nullptr)
{
	using namespace Ui::LayoutEngine;

	auto root = Ui::UiBuilder<Ui::Container>::create();

	root.withGrid({Track::flex(1.0f)}, {Track::fix(LayoutConstants::headerHeight), Track::fix(LayoutConstants::menuGap),
	                                    Track::flex(1.0f), Track::fix(LayoutConstants::footerHeight)})
	    .add(Ui::Label(titleText, ctx.titleFont, ctx.theme.titleTextColor),
	         {.row = 0, .col = 0, .hAlign = Alignment::Center, .vAlign = Alignment::Center});

	root.add(std::move(contentBuilder), {.row = 2,
	                                     .col = 0,
	                                     .hAlign = Alignment::Center,
	                                     .vAlign = Alignment::Stretch,
	                                     .minSize = {contentWidth, 0.0f}});

	if (onBottomButton && !bottomButtonText.empty()) {
		root.add(Ui::Button(bottomButtonText, ctx.bigFont, onBottomButton),
		         {.row = 3, .col = 0, .hAlign = Alignment::Center, .vAlign = Alignment::Center});
	}
	return root.build(&ctx, getScreenRect(ctx));
}
} // namespace

namespace UiViewFactory {

using namespace Ui;
using namespace Ui::LayoutEngine;

std::unique_ptr<Widget> createMainMenu(const UIContext &ctx, const MainMenuActions &actions)
{
	auto content = UiBuilder<Container>::create();
	content.withLinear(Orientation::Vertical, LayoutConstants::gap)
	    .add(Button("Start New Game", ctx.bigFont, actions.onStartNewGame), {.hAlign = Alignment::Stretch})
	    .add(Button("Continue", ctx.bigFont, actions.onShowContinue), {.hAlign = Alignment::Stretch})
	    .add(Button("Settings", ctx.bigFont, actions.onShowSettings), {.hAlign = Alignment::Stretch})
	    .add(Button("Exit", ctx.bigFont, actions.onQuit), {.hAlign = Alignment::Stretch});

	return createStandardScreen(ctx, "Coredump Castle", std::move(content), LayoutConstants::buttonMenuWidth);
}

std::unique_ptr<Widget> createPauseMenu(const UIContext &ctx, const PauseMenuActions &actions)
{
	auto content = UiBuilder<Container>::create();
	content.withLinear(Orientation::Vertical, LayoutConstants::gap)
	    .add(Button("Resume", ctx.bigFont, actions.onResume), {.hAlign = Alignment::Stretch})
	    .add(Button("Inventory", ctx.bigFont, actions.onShowInventory), {.hAlign = Alignment::Stretch})
	    .add(Button("Settings", ctx.bigFont, actions.onShowSettings), {.hAlign = Alignment::Stretch})
	    .add(Button("Quit", ctx.bigFont, actions.onBackToMainMenu), {.hAlign = Alignment::Stretch});

	return createStandardScreen(ctx, "Game Paused", std::move(content), LayoutConstants::buttonMenuWidth);
}

std::unique_ptr<Widget> createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
                                           const std::vector<GameEngine::ECS::SaveMetaData> &savesData)
{
	const std::vector gridRows(savesData.size(), Track::fix(30.0f));

	auto saveListBuilder = UiBuilder<Container>::create();
	saveListBuilder.withGrid({Track::flex(1.0f), Track::fix(120.0f)}, gridRows, 4.0f);

	int currentRow = 0;
	for (const auto &saveData : savesData) {
		std::string labelText = GameEngine::Utils::SaveGameFormatter::getDisplayString(saveData);
		saveListBuilder.add(Label(labelText, ctx.textFont), {.row = currentRow, .col = 0, .hAlign = Alignment::Start});

		auto loadCallback = [actions, name = saveData.saveName] { actions.onContinue(name); };
		saveListBuilder.add(Button("Continue", ctx.bigFont, loadCallback),
		                    {.row = currentRow, .col = 1, .hAlign = Alignment::Stretch});
		currentRow++;
	}

	return createStandardScreen(ctx, "Continue Game", std::move(saveListBuilder), LayoutConstants::panelContentWidth,
	                            "Back", actions.onBack);
}

std::unique_ptr<Widget> createSettingsMenu(const UIContext &ctx, const SettingsMenuActions &actions,
                                           const bool pauseMenuView)
{
	std::unordered_map<std::string, std::string> keyMapping = {
	    {"Left", "[A]"},     {"Right", "[D]"},      {"Jump", "[Space]"},           {"Attack", "[K]"},
	    {"Interact", "[E]"}, {"Toggle Map", "[M]"}, {"Toggle Pause Menu", "[ESC]"}};

	const int rowCount = static_cast<int>(keyMapping.size()) + (pauseMenuView ? 0 : 2);
	const std::vector rows(rowCount, Track::fix(20.0f));

	auto contentBuilder = UiBuilder<Container>::create();
	contentBuilder.withGrid({Track::flex(1.0f), Track::fix(50.0f)}, rows);

	int currentRow = 0;
	for (const auto &[action, key] : keyMapping) {
		contentBuilder.add(Label(action, ctx.textFont), {.row = currentRow, .col = 0, .hAlign = Alignment::Start});
		contentBuilder.add(Label(key, ctx.textFont), {.row = currentRow, .col = 1, .hAlign = Alignment::Center});
		currentRow++;
	}

	if (!pauseMenuView) {
		contentBuilder.add(Frame(), {.row = currentRow++, .spanX = 2, .minSize{LayoutConstants::panelContentWidth, 2}});
		contentBuilder.add(Label("Delete all saves", ctx.textFont),
		                   {.row = currentRow, .col = 0, .hAlign = Alignment::Start});
		contentBuilder.add(Button("Delete", ctx.textFont, actions.onDelete),
		                   {.row = currentRow, .col = 1, .hAlign = Alignment::End});
	}

	return createStandardScreen(ctx, "Settings", std::move(contentBuilder), LayoutConstants::panelContentWidth, "Back",
	                            actions.onBack);
}

std::unique_ptr<Widget> createInventoryMenu(const UIContext &ctx, const InventoryActions &actions, int collectedKeys,
                                            const std::vector<int> &unlockedAbilities, TextureManager &textureManager)
{
	SDL_Texture *keyTex = &textureManager.getTexture("assets/tiled/tilesets/Ui/ui_key.png");
	SDL_Texture *slotTex = &textureManager.getTexture("assets/tiled/tilesets/Ui/Inventory_Slot_1.png");
	SDL_Texture *swordTex = &textureManager.getTexture("assets/tiled/tilesets/Ui/sword_pixel.png");
	SDL_Texture *shoeTex =
	    &(unlockedAbilities.size() < 1 ? textureManager.getTexture("assets/tiled/tilesets/Ui/default_boots.png")
	                                   : textureManager.getTexture("assets/tiled/tilesets/Ui/doublejump_boots.png"));

	auto createSlot = [&](SDL_Texture *background, SDL_Texture *item) {
		auto builder = UiBuilder<Container>::create();
		builder.withStack().add(Image(background), {.hAlign = Alignment::Stretch, .vAlign = Alignment::Stretch});
		if (item) {
			builder.add(Image(item), {.hAlign = Alignment::Center, .vAlign = Alignment::Center, .minSize = {30, 30}});
		}
		return builder;
	};

	auto invBuilder = UiBuilder<Container>::create();
	invBuilder.withGrid({Track::fix(60.0f), Track::fix(60.0f)},
	                    {Track::fix(25.0f), Track::fix(60.0f), Track::fix(25.0f), Track::fix(60.0f)});

	invBuilder.add(Label("Items", ctx.textFont), {.row = 0, .col = 0, .spanX = 2, .hAlign = Alignment::Start})
	    .add(createSlot(slotTex, swordTex),
	         {.row = 1, .col = 0, .hAlign = Alignment::Stretch, .vAlign = Alignment::Stretch})
	    .add(createSlot(slotTex, shoeTex),
	         {.row = 1, .col = 1, .hAlign = Alignment::Stretch, .vAlign = Alignment::Stretch})
	    .add(Label("Keys", ctx.textFont), {.row = 2, .col = 0, .spanX = 2, .hAlign = Alignment::Start})
	    .add(createSlot(slotTex, collectedKeys >= 1 ? keyTex : nullptr),
	         {.row = 3, .col = 0, .hAlign = Alignment::Stretch, .vAlign = Alignment::Stretch})
	    .add(createSlot(slotTex, collectedKeys >= 2 ? keyTex : nullptr),
	         {.row = 3, .col = 1, .hAlign = Alignment::Stretch, .vAlign = Alignment::Stretch});

	auto inventoryPanel = UiBuilder<Container>::create();
	inventoryPanel.withStack()
	    .add(Frame(), {.minSize = {400, 200}, .anchor = {AnchorParams::center}})
	    .add(std::move(invBuilder),
	         {.hAlign = Alignment::Center, .vAlign = Alignment::Center, .minSize = {120.0f, 170.0f}});

	return createStandardScreen(ctx, "Inventory", std::move(inventoryPanel), LayoutConstants::panelContentWidth, "Back",
	                            actions.onBack);
}

std::unique_ptr<Widget> createLoadingScreen(const UIContext &ctx)
{
	return UiBuilder<Container>::create()
	    .withStack()
	    .add(Label("Loading...", ctx.bigFont), {.anchor = AnchorParams::bottomCenter})
	    .build(&ctx, getScreenRect(ctx));
}

std::unique_ptr<Widget> createMapScreen(const UIContext &ctx, const float mapSizeFactor)
{
	static constexpr int bleedingGuard = 3;
	const SDL_FRect screen = getScreenRect(ctx);

	return UiBuilder<Container>::create()
	    .withStack()
	    .add(Frame(), {.minSize = {screen.w * mapSizeFactor + bleedingGuard, screen.h * mapSizeFactor + bleedingGuard},
	                   .anchor = {AnchorParams::center}})
	    .build(&ctx, screen);
}

std::unique_ptr<Widget> createGameOverScreen(const UIContext &ctx, GameEngine::GameEngine &game)
{
	const std::string hintString = std::format("Press [{}] to reload. Press [{}] to return to the main menu.",
	                                           game.getInputManager().getHumanReadableKey(Key::Up),
	                                           game.getInputManager().getHumanReadableKey(Key::Escape));

	return UiBuilder<Container>::create()
	    .withStack()
	    .add(Label("Game Over", ctx.titleFont, ctx.theme.titleTextColor), {.anchor = {AnchorParams::topCenter}})
	    .add(Label(hintString, ctx.textFont), {.anchor = AnchorParams::bottomCenter})
	    .build(&ctx, getScreenRect(ctx));
}
} // namespace UiViewFactory
