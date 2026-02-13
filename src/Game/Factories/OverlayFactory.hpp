#pragma once

#include "Engine/Graphics/TextureManager.hpp"
#include "SDL3/SDL_rect.h"
#include "TileFactory.hpp"
#include "tileson.hpp"

#include <glm/vec2.hpp>
#include <string>
namespace GameEngine::ECS {
class Registry;
}

struct UiElementData {
	SDL_FRect hitbox{0, 0, 3.0f, 3.0f};
	glm::ivec2 tileSize{32, 32};
	std::string texturePath{};
};

class OverlayFactory {
  public:
	OverlayFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager);

	OverlayFactory(const OverlayFactory &) = delete;
	OverlayFactory &operator=(const OverlayFactory &) = delete;
	OverlayFactory(OverlayFactory &&) = delete;
	OverlayFactory &operator=(OverlayFactory &&) = delete;
	~OverlayFactory() = default;

	void createOverlay(const std::filesystem::path &path);

  private:
	void loadObjects(tson::Layer &layer, const int &layerIndex) const;

	GameEngine::ECS::Registry &registry;
	AnimationFactory animationFactory;
	TileFactory tileFactory;
};