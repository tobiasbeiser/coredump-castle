#pragma once
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Entity.hpp"
#include "tileson.hpp"

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

class AnimationFactory;
namespace GameEngine::ECS {
class Registry;
}

class EnemySpawner {
  public:
	explicit EnemySpawner(GameEngine::ECS::Registry &registry);
	EnemySpawner(const EnemySpawner &) = delete;
	EnemySpawner &operator=(const EnemySpawner &) = delete;
	EnemySpawner(EnemySpawner &&) = default;
	EnemySpawner &operator=(EnemySpawner &&) = delete;
	~EnemySpawner() = default;

	void spawn();
	void despawn();
	[[nodiscard]] bool isAlive() const;

	template <typename T>
	void addBlueprint(T component);

  private:
	GameEngine::ECS::Registry &registry;
	GameEngine::ECS::Entity currentEntity;

	using ComponentAdder = std::function<void(GameEngine::ECS::Entity)>;
	std::vector<ComponentAdder> blueprints;
};