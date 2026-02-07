#pragma once
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "tileson.hpp"

#include <vector>

namespace GameEngine::ECS {
class Registry;
}

class AnimationFactory {
  public:
	AnimationFactory(TextureManager &textureManager) : textureManager(textureManager) {}

	AnimationFactory(const AnimationFactory &) = delete;
	AnimationFactory &operator=(const AnimationFactory &) = delete;
	AnimationFactory(AnimationFactory &&) = delete;
	AnimationFactory &operator=(AnimationFactory &&) = delete;
	~AnimationFactory() = default;

	void setMap(tson::Map &tsonMap) { map = &tsonMap; }
	static AnimationType getAnimationType(const std::string &string);
	static AnimationType getAnimationTypeFromId(int id);

	std::optional<std::tuple<AnimationComponent, SpriteComponent>>
	loadAnimationsFromTileset(tson::Object &object) const;

	std::optional<std::tuple<AnimationComponent, SpriteComponent>>
	loadAnimationFromStandaloneTileset(tson::Tileset &tileset, const std::string &tilesetPath) const;

	std::optional<std::tuple<AnimationComponent, SpriteComponent>>
	loadProjectileAnimation(const std::string &tilesetPath) const;

	AnimationComponent loadTileAnimation(tson::Tile *const &tile) const;

  private:
	TextureManager &textureManager;
	tson::Map *map = nullptr;
	const std::filesystem::path basePath = "assets/tiled/tilesets/";

	struct AnimationData {
		AnimationType type;
		std::vector<AnimationFrame> frames;
		std::vector<bool> damageFrames;
		std::chrono::duration<float> duration;
	};

	AnimationData loadAnimation(tson::Tile *const &tile) const;
};