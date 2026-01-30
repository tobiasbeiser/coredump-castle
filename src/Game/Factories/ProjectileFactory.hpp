#pragma once

#include "AnimationFactory.hpp"
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
	glm::ivec2 tileSize{32, 32};
	std::string texturePath{};
};

class ProjectileFactory {
  public:
	ProjectileFactory(GameEngine::ECS::Registry &registry, AnimationFactory &animationFactory);

	ProjectileFactory(const ProjectileFactory &) = delete;
	ProjectileFactory &operator=(const ProjectileFactory &) = delete;
	ProjectileFactory(ProjectileFactory &&) = delete;
	ProjectileFactory &operator=(ProjectileFactory &&) = delete;
	~ProjectileFactory() = default;

	void createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, float speed, float range,
	                      const std::string &tilesetPath, double rotation = 0.0) const;

	static const ProjectileData &getCachedProjectileData(const std::string &path);

  private:
	GameEngine::ECS::Registry &registry;
	AnimationFactory &animationFactory;
};