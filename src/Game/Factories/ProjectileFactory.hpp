#pragma once

#include "SDL3/SDL_rect.h"
#include <functional>
#include <glm/vec2.hpp>
#include <string>
namespace GameEngine::ECS {
class Registry;
}
class TextureManager;

struct ProjectileData {
	SDL_FRect hitbox{0, 0, 3.0f, 3.0f};
	glm::vec2 spriteSize{32.0f, 32.0f};
};

class ProjectileFactory {
  public:
	ProjectileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager);

	void createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, float speed, float range,
	                      const std::string &tilesetPath, double rotation = 0.0) const;

	static const ProjectileData &getCachedProjectileData(const std::string &path);

  private:
	std::reference_wrapper<GameEngine::ECS::Registry> registry;
	std::reference_wrapper<TextureManager> textureManager;
};