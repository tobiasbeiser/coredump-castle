#pragma once
#include "GuiManager.hpp"
#include <functional>

#include "Engine/FileIO/ISaveStorage.hpp"

namespace GameEngine {
class GameEngine;
}

struct MainMenuActions {
	std::function<void()> onStartNewGame;
	std::function<void()> onShowContinue;
	std::function<void()> onShowSettings;
	std::function<void()> onQuit;
};

struct PauseMenuActions {
	std::function<void()> onShowInventory;
	std::function<void()> onShowSettings;
	std::function<void()> onBackToMainMenu;
};

struct ContinueMenuActions {
	std::function<void(std::string_view)> onContinue;
	std::function<void()> onBack;
};

struct SettingsMenuActions {
	std::function<void()> onBack;
	std::function<void()> onDelete;
};

struct InventoryActions {
	std::function<void()> onBack;
};

namespace UiViewFactory {
using namespace Ui;
std::unique_ptr<Widget> createMainMenu(const UIContext &ctx, const MainMenuActions &actions);

std::unique_ptr<Widget> createPauseMenu(const UIContext &ctx, const PauseMenuActions &actions);

std::unique_ptr<Widget> createContinueMenu(const UIContext &ctx, const ContinueMenuActions &actions,
                                           const std::vector<GameEngine::ECS::SaveMetaData>& savesData);

std::unique_ptr<Widget> createSettingsMenu(const UIContext &ctx, const SettingsMenuActions &actions, bool pauseMenuView=false);

std::unique_ptr<Widget> createInventoryMenu(const UIContext &ctx, const InventoryActions &actions, int collectedKeys, const std::vector<int> &unlockedAbilities);

std::unique_ptr<Widget> createLoadingScreen(const UIContext &ctx);

std::unique_ptr<Widget> createMapScreen(const UIContext &ctx, float mapSizeFactor);

std::unique_ptr<Widget> createGameOverScreen(const UIContext &ctx, GameEngine::GameEngine &game);
}; // namespace UiViewFactory