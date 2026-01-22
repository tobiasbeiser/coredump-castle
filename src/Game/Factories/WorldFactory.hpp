#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Engine/Input/InputManager.hpp"

#include <filesystem>
#include <tileson.hpp>

class WorldFactory {
  public:
	WorldFactory(GameEngine::ECS::Registry &registry, const std::filesystem::path &projectPath,
	             TextureManager &textureManager, InputManager &inputManager);

	WorldFactory(const WorldFactory &) = delete;
	WorldFactory &operator=(const WorldFactory &) = delete;
	WorldFactory(WorldFactory &&) = delete;
	WorldFactory &operator=(WorldFactory &&) = delete;
	~WorldFactory() = default;

	void createWorld(const std::filesystem::path &worldPath);

  private:
	GameEngine::ECS::Registry &registry;
	tson::Project project{};
	tson::Tileson tileson{&project};
	TextureManager &textureManager;
	InputManager &inputManager;
};
