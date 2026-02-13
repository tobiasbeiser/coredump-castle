#pragma once
#include "Audio/AudioManager.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/SaveSystem.hpp"
#include "Game/Factories/EffectFactory.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include "Graphics/FontManager.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureManager.hpp"
#include "Graphics/Gui/Core/UiContext.hpp"
#include "Input/InputManager.hpp"
#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {
class GameState;

template <typename T>
concept derivedFromGameState = std::derived_from<T, GameState>;

class GameEngine {
  public:
	GameEngine(const std::string &title, Resolution resolution);
	~GameEngine();
	// delete copy, move and assignments as we want only one instance of the game engine
	GameEngine(const GameEngine &) = delete;
	GameEngine &operator=(const GameEngine &) = delete;
	GameEngine(GameEngine &&) = delete;
	GameEngine &operator=(GameEngine &&) = delete;
	bool init();
	void cleanup() noexcept;

	void changeState(std::unique_ptr<GameState> state);
	void pushState(std::unique_ptr<GameState> state);
	void popState();
	void resetToState(std::unique_ptr<GameState> state);

	void handleEvents();
	void update(float deltaTime);
	void render(float alpha);

	template <derivedFromGameState T>
	T &getState()
	{
		for (auto &state : states) {
			if (auto casted = dynamic_cast<T *>(state.get())) {
				return *casted;
			}
		}
		throw std::runtime_error("State of requested type not found");
	}

	bool isRunning() const { return running; }
	void quit() { running = false; }

	Renderer &getRenderer() { return renderer; }
	ECS::Registry &getRegistry() { return registry; }
	InputManager &getInputManager() { return inputManager; }
	TextureManager &getTextureManager() { return textureManager; }
	FontManager &getFontManager() { return fontManager; }
	Ui::UIContext &getUiContext() const { return *context; }
	EffectFactory &getEffectFactory() { return effectFactory; };
	ECS::Systems::SaveSystem &getSaveSystem() { return saveSystem; }
	AudioManager &getAudioManager() { return audioManager; }
	ProjectileFactory &getProjectileFactory() { return projectileFactory; }

  private:
	std::vector<std::unique_ptr<GameState>> states{};
	bool running = false;
	const std::string title;
	ECS::Registry registry;
	Renderer renderer;
	InputManager inputManager;
	TextureManager textureManager;
	FontManager fontManager;
	std::unique_ptr<Ui::UIContext> context;
	ECS::Systems::SaveSystem saveSystem;
	struct PendingAction {
		enum class Type { Push, Pop, Change, Reset};
		Type type;
		std::optional<std::unique_ptr<GameState>> state;
	};
	std::vector<PendingAction> actionQueue;
	EffectFactory effectFactory{registry, textureManager};
	AnimationFactory animationFactory{textureManager};
	ProjectileFactory projectileFactory{registry, animationFactory};
	AudioManager audioManager{};
};
} // namespace GameEngine