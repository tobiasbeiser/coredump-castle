#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <filesystem>
#include <tileson.hpp>

class WorldFactory {
  public:
	WorldFactory(GameEngine::ECS::Registry &registry, const std::filesystem::path &projectPath, TextureManager &textureManager);

	void CreateWorld(const std::filesystem::path &worldPath);

  private:
	GameEngine::ECS::Registry &registry;
	tson::Project project;
	tson::Tileson tileson{&project};
	TextureManager &textureManager;
};
