
#include "UiViewFactory.hpp"

inline std::unique_ptr<Button> makeWrappingButton(const UIContext &ctx, std::string_view text, std::function<void()> cb)
{
	auto btn = std::make_unique<Button>(text, ctx.bigFont, std::move(cb));
	return btn;
}

inline std::unique_ptr<Button> makeBigMenuButton(const UIContext &ctx, std::string_view text, std::function<void()> cb)
{
	auto btn = std::make_unique<Button>(text, ctx.bigFont, std::move(cb));
	btn->setFixedSize({200.0f, 40.0f});
	btn->setSizeMode(SizeMode::Fixed, SizeMode::Fixed);
	return btn;
}

inline std::unique_ptr<VBox> makeMenuRoot(const UIContext &ctx, std::string_view titleText)
{
	auto root = std::make_unique<VBox>(10.0f, 0.0f);
	root->setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent);

	root->addWidget(std::make_unique<Spacer>(ctx.resolution.height * 0.1f));

	auto titleLabel = std::make_unique<Label>(titleText, ctx.titleFont);
	titleLabel->setColor(ctx.theme.titleTextColor);
	root->addWidget(std::move(titleLabel));

	root->addWidget(std::make_unique<Spacer>(ctx.resolution.height * 0.1f));
	return root;
}

std::unique_ptr<Widget> UiViewFactory::createMainMenu(const UIContext &ctx, const MainMenuActions &actions)
{
	auto root = makeMenuRoot(ctx, "Coredump Castle");

	if (actions.onStartNewGame) {
		root->addWidget(makeBigMenuButton(ctx, "Start New Game", actions.onStartNewGame));
	}

	if (actions.onShowContinue) {
		root->addWidget(makeBigMenuButton(ctx, "Continue", actions.onShowContinue));
	}

	if (actions.onShowOptions) {
		root->addWidget(makeBigMenuButton(ctx, "Options", actions.onShowOptions));
	}

	if (actions.onQuit) {
		root->addWidget(makeBigMenuButton(ctx, "Quit", actions.onQuit));
	}

	return root;
}

std::unique_ptr<Widget> UiViewFactory::createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
                                                          std::vector<std::string> saveNames)
{
	auto root = makeMenuRoot(ctx, "Continue Game");

	for (const auto saveName : saveNames) {
		auto loadCallback = [actions, saveName] { actions.onContinue(saveName); };
		root->addWidget(makeBigMenuButton(ctx, saveName, loadCallback));
	}

	if (actions.onBack) {
		root->addWidget(makeWrappingButton(ctx, "Back", actions.onBack));
	}

	return root;
}

std::unique_ptr<Widget> UiViewFactory::createOptionsMenu(const UIContext &ctx, const OptionsMenuActions &actions)
{
	auto root = makeMenuRoot(ctx, "Options");

	// Example Option
	root->addWidget(std::make_unique<Label>("Volume: 100%", ctx.bigFont));

	root->addWidget(makeWrappingButton(ctx, "Back", actions.onBack));
	return root;
}
