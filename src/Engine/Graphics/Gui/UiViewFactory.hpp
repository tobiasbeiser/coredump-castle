#pragma once
#include "GuiManager.hpp"
#include <functional>
#include <string>

struct MainMenuActions {
	std::function<void()> onStartNewGame;
	std::function<void()> onShowContinue;
	std::function<void()> onShowOptions;
	std::function<void()> onQuit;
};

struct ContinueMenuActions {
	std::function<void(std::string_view)> onContinue;
	std::function<void()> onBack;
};

struct OptionsMenuActions {
	std::function<void()> onBack;
	// Add volume toggles etc here
};

namespace UiViewFactory {
	using namespace Ui;
	std::unique_ptr<Widget> createMainMenu(const UIContext &ctx, const MainMenuActions &actions);

	std::unique_ptr<Widget> createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
											   std::vector<std::string> saveNames);

	std::unique_ptr<Widget> createOptionsMenu(const UIContext &ctx, const OptionsMenuActions &actions);
}; // namespace UiViewFactory