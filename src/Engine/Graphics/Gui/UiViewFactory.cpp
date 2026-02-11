
#include "UiViewFactory.hpp"

#include "Engine/GameEngine.hpp"

#include <utility>

#include "Engine/Graphics/TextureManager.hpp"
#include "Engine/Utils/SaveGameFormatter.hpp"
#include "Layouts/AnchorLayout.hpp"
#include "Layouts/LinearLayout.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/Frame.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Spacer.hpp"

namespace UiViewFactory {
inline std::unique_ptr<Spacer> makeSpacer(const float layoutWeight = 1.0f)
{
	auto spacer = std::make_unique<Spacer>();
	spacer->setLayoutWeight(std::clamp(layoutWeight, 0.f, 1.0f));
	return spacer;
}

inline std::unique_ptr<Button> makeWrappingButton(const UIContext &ctx, std::string_view text, std::function<void()> cb)
{
	auto btn = std::make_unique<Button>(text, ctx.bigFont, std::move(cb));
	return btn;
}

inline std::unique_ptr<Button> makeBigMenuButton(const UIContext &ctx, std::string_view text, std::function<void()> cb)
{
	auto btn = std::make_unique<Button>(text, ctx.bigFont, std::move(cb));
	btn->setSizeMode(SizeMode::MatchParent, SizeMode::WrapContent);
	return btn;
}

inline std::unique_ptr<LinearLayout> makeMenuRoot(const UIContext &ctx, std::string_view titleText)
{
	auto root = std::make_unique<LinearLayout>();
	root->setSpacing(0.0f);
	root->setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent);

	root->addWidget(std::make_unique<Spacer>(0.f, ctx.resolution.height * 0.1f));

	auto titleLabel = std::make_unique<Label>(titleText, ctx.titleFont);
	titleLabel->setColor(ctx.theme.titleTextColor);
	root->addWidget(std::move(titleLabel));

	root->addWidget(std::make_unique<Spacer>(0.f, ctx.resolution.height * 0.1f));
	return root;
}

inline std::unique_ptr<LinearLayout> makeBackButtonFooter(const UIContext &ctx, std::function<void()> cb)
{
	auto footer = std::make_unique<LinearLayout>();
	footer->setOrientation(Orientation::Horizontal);
	footer->setCrossAxisAlignment(CrossAxisAlignment::Center);

	footer->addWidget(makeSpacer());

	std::unique_ptr<Button> backButton = makeWrappingButton(ctx, "Back", std::move(cb));
	footer->addWidget(std::move(backButton));
	return footer;
}

inline std::unique_ptr<LinearLayout> makeButtonContainer()
{
	auto buttonContainer = std::make_unique<LinearLayout>();
	buttonContainer->setSpacing(8.0f);
	buttonContainer->setCrossAxisAlignment(CrossAxisAlignment::Center);
	buttonContainer->setSizeMode(SizeMode::WrapContent, SizeMode::MatchParent);
	return buttonContainer;
}

std::unique_ptr<Widget> createMainMenu(const UIContext &ctx, const MainMenuActions &actions)
{
	auto root = makeMenuRoot(ctx, "Coredump Castle");

	auto buttonContainer = makeButtonContainer();

	if (actions.onStartNewGame) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Start New Game", actions.onStartNewGame));
	}

	auto bigMenuButton = makeBigMenuButton(ctx, "Continue", actions.onShowContinue);
	if (!actions.onShowContinue) {
		bigMenuButton->setButtonDisabled(true);
	}
	buttonContainer->addWidget(std::move(bigMenuButton));

	if (actions.onShowSettings) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Settings", actions.onShowSettings));
	}

	if (actions.onQuit) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Quit", actions.onQuit));
	}

	root->addWidget(std::move(buttonContainer));

	return root;
}

std::unique_ptr<Widget> createPauseMenu(const UIContext &ctx, const PauseMenuActions &actions)
{
	auto root = makeMenuRoot(ctx, "Game Paused");

	auto buttonContainer = makeButtonContainer();

	if (actions.onShowInventory) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Inventory", actions.onShowInventory));
	}

	if (actions.onShowSettings) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Settings", actions.onShowSettings));
	}

	if (actions.onBackToMainMenu) {
		buttonContainer->addWidget(makeBigMenuButton(ctx, "Main Menu", actions.onBackToMainMenu));
	}

	root->addWidget(std::move(buttonContainer));

	return root;
}

std::unique_ptr<Widget> createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
                                           const std::vector<GameEngine::ECS::SaveMetaData>& savesData)
{
	auto root = makeMenuRoot(ctx, "Continue Game");

	auto posContainer = std::make_unique<LinearLayout>();
	posContainer->setOrientation(Orientation::Horizontal);
	posContainer->addWidget(makeSpacer());

	auto buttonContainer = makeButtonContainer();

	for (const auto& saveData : savesData) {
		auto buttonRow = std::make_unique<LinearLayout>();
		buttonRow->setOrientation(Orientation::Horizontal);

		auto metaLabel =
		    std::make_unique<Label>(GameEngine::Utils::SaveGameFormatter::getDisplayString(saveData), ctx.textFont);
		buttonRow->addWidget(std::move(metaLabel));
		auto loadCallback = [actions, saveData] { actions.onContinue(saveData.saveName); };
		buttonRow->addWidget(makeWrappingButton(ctx, "continue", loadCallback));
		buttonContainer->addWidget(std::move(buttonRow));
	}

	auto continueContainer = std::make_unique<LinearLayout>();

	continueContainer->setSpacing(20.f);
	continueContainer->addWidget(std::move(buttonContainer));
	continueContainer->setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);

	if (actions.onBack) {
		continueContainer->addWidget(makeBackButtonFooter(ctx, actions.onBack));
	}
	posContainer->addWidget(std::move(continueContainer));
	posContainer->addWidget(makeSpacer());

	root->addWidget(std::move(posContainer));

	return root;
}

std::unique_ptr<Widget> createSettingsMenu(const UIContext &ctx, const SettingsMenuActions &actions, const bool pauseMenuView)
{
	auto root = makeMenuRoot(ctx, "Settings");

	std::unordered_map<std::string, std::string> keyMapping = {{"Left", "[A]"},     {"Right", "[D]"},
	                                                           {"Jump", "[Space]"}, {"Attack", "[K]"},
	                                                           {"Interact", "[E]"}, {"Toggle Map", "[M]"}};

	auto container = std::make_unique<LinearLayout>();
	container->setOrientation(Orientation::Horizontal);

	container->addWidget(makeSpacer());

	auto content = std::make_unique<LinearLayout>();
	content->setLayoutWeight(1.0f);
	content->setCrossAxisAlignment(CrossAxisAlignment::Stretch);
	for (const auto &[action, key] : keyMapping) {
		auto listElement = std::make_unique<LinearLayout>();
		listElement->setOrientation(Orientation::Horizontal);
		auto actionLabel = std::make_unique<Label>(action, ctx.textFont);
		actionLabel->setLayoutWeight(0.5);
		listElement->addWidget(std::move(actionLabel));
		listElement->addWidget(makeSpacer(0.8));
		auto keyLabel = std::make_unique<Label>(key, ctx.textFont);
		keyLabel->setLayoutWeight(0.30f);
		listElement->addWidget(std::move(keyLabel));

		content->addWidget(std::move(listElement));
	}

	if (!pauseMenuView) {
		auto deleteAll = std::make_unique<LinearLayout>();
		deleteAll->setOrientation(Orientation::Horizontal);
		auto deleteLabel = std::make_unique<Label>("Delete all saves", ctx.textFont);
		deleteLabel->setLayoutWeight(1.0f);
		deleteAll->addWidget(std::move(deleteLabel));

		auto deleteButton = std::make_unique<Button>("Delete", ctx.textFont, actions.onDelete);
		if (!actions.onDelete) {
			deleteButton->setButtonDisabled(true);
		}

		deleteAll->addWidget(std::move(deleteButton));
		content->addWidget(std::move(deleteAll));
	}
	content->addWidget(makeBackButtonFooter(ctx, actions.onBack));

	container->addWidget(std::move(content));
	container->addWidget(makeSpacer());

	root->addWidget(std::move(container));
	return root;
}

std::unique_ptr<Widget> createInventoryMenu(const UIContext &ctx, const InventoryActions &actions, int collectedKeys,
	const std::vector<int> &unlockedAbilities)
{
	auto root = makeMenuRoot(ctx, "Inventory");

	auto posContainer = std::make_unique<LinearLayout>();
	posContainer->setOrientation(Orientation::Horizontal);
	posContainer->addWidget(makeSpacer());

	root->addWidget(makeBackButtonFooter(ctx, actions.onBack));
	return root;
}

std::unique_ptr<Widget> createLoadingScreen(const UIContext &ctx)
{
	auto root = std::make_unique<AnchorLayout>();
	auto loadingLabel = std::make_unique<Label>("Loading...", ctx.bigFont);
	root->addChild(std::move(loadingLabel), AnchorParams::bottomCenter);
	return root;
}

std::unique_ptr<Widget> createMapScreen(const UIContext &ctx, const float mapSizeFactor)
{
	static constexpr int bleedingGuard = 3;
	const auto [width, height] = ctx.renderer->getLogicalResolution();
	auto root = std::make_unique<AnchorLayout>();

	auto mapContent = std::make_unique<Frame>();
	mapContent->setFixedSize({width * mapSizeFactor + bleedingGuard, height * mapSizeFactor + bleedingGuard});
	root->addChild(std::move(mapContent), AnchorParams::center);

	return root;
}

std::unique_ptr<Widget> createGameOverScreen(const UIContext &ctx, GameEngine::GameEngine &game)
{
	const auto [width, height] = ctx.renderer->getLogicalResolution();
	auto root = std::make_unique<AnchorLayout>();

	auto title = std::make_unique<Label>("Game Over", ctx.titleFont);
	root->addChild(std::move(title), {{0.5f, 0.1f}, {0.5f, 0.5f}, {0.0f, 0.0f}});

	std::string hintString = std::format("Press [{}] to reload. Press [{}] to return to the main menu.", game.getInputManager().getHumanReadableKey(Key::Up), game.getInputManager().getHumanReadableKey(Key::Escape));
	auto hint = std::make_unique<Label>(hintString, ctx.textFont);
	root->addChild(std::move(hint), {{0.5f, 1.0f}, {0.5f, 1.0f}, {0.0f, -10.0f}});

	return root;
}
} // namespace UiViewFactory
