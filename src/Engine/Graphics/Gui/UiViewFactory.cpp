
#include "UiViewFactory.hpp"

#include <ranges>
#include <utility>

#include "Layouts/LinearLayout.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Spacer.hpp"

namespace UiViewFactory
{
	inline std::unique_ptr<Spacer> makeSpacer(const float layoutWeight = 1.0f)
	{
		auto btn = std::make_unique<Spacer>();
		btn->setLayoutWeight(std::clamp(layoutWeight, 0.f, 1.0f));
		return btn;
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

		if (actions.onShowContinue) {
			buttonContainer->addWidget(makeBigMenuButton(ctx, "Continue", actions.onShowContinue));
		} else {
			auto bigMenuButton = makeBigMenuButton(ctx, "Continue", []{});
			bigMenuButton->setButtonDisabled(true);
			buttonContainer->addWidget(std::move(bigMenuButton));
		}

		if (actions.onShowOptions) {
			buttonContainer->addWidget(makeBigMenuButton(ctx, "Options", actions.onShowOptions));
		}

		if (actions.onQuit) {
			buttonContainer->addWidget(makeBigMenuButton(ctx, "Quit", actions.onQuit));
		}

		root->addWidget(std::move(buttonContainer));

		return root;
	}

	std::unique_ptr<Widget> createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
															  std::vector<std::string> saveNames)
	{
		auto root = makeMenuRoot(ctx, "Continue Game");

		auto posContainer = std::make_unique<LinearLayout>();
		posContainer->setOrientation(Orientation::Horizontal);
		posContainer->addWidget(makeSpacer());


		auto buttonContainer = makeButtonContainer();

		for (const auto saveName : saveNames) {
			auto loadCallback = [actions, saveName] { actions.onContinue(saveName); };
			buttonContainer->addWidget(makeBigMenuButton(ctx, saveName, loadCallback));
		}

		auto continueContainer = std::make_unique<LinearLayout>();

		continueContainer->setSpacing(20.f);
		continueContainer->addWidget(std::move(buttonContainer));
		continueContainer->setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);

		if (actions.onBack) {
			continueContainer->addWidget(makeBackButtonFooter(ctx, actions.onBack));
		}
		continueContainer->setLayoutWeight(0.6f);
		posContainer->addWidget(std::move(continueContainer));
		posContainer->addWidget(makeSpacer());

		root->addWidget(std::move(posContainer));

		return root;
	}

	std::unique_ptr<Widget> createOptionsMenu(const UIContext &ctx, const OptionsMenuActions &actions)
	{
		auto root = makeMenuRoot(ctx, "Controls");

		std::unordered_map<std::string, std::string> keyMapping = {
			{"Left", "[A]"}, {"Right", "[D]"}, {"Jump", "[Space]"}, {"Attack", "[K]"}, {"Interact", "[E]"},
		};

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
			actionLabel->setLayoutWeight(0.40f);
			listElement->addWidget(std::move(actionLabel));
			listElement->addWidget(makeSpacer());
			auto keyLabel = std::make_unique<Label>(key, ctx.textFont);
			keyLabel->setLayoutWeight(0.35f);
			listElement->addWidget(std::move(keyLabel));

			content->addWidget(std::move(listElement));
		}

		content->addWidget(makeBackButtonFooter(ctx, actions.onBack));

		container->addWidget(std::move(content));

		container->addWidget(makeSpacer());

		root->addWidget(std::move(container));
		return root;
	}
}
