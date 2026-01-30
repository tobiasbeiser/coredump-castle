#pragma once
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "tileson.hpp"

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

class AnimationFactory;

class EnemySpawner {
  public:
	explicit EnemySpawner(GameEngine::ECS::Registry &registry);
	EnemySpawner(const EnemySpawner &) = delete;
	EnemySpawner &operator=(const EnemySpawner &) = delete;
	EnemySpawner(EnemySpawner &&) = default;
	EnemySpawner &operator=(EnemySpawner &&) = delete;
	~EnemySpawner() = default;

	void spawn();
	void despawn() const;
	[[nodiscard]] bool isAlive() const;

	template <typename T>
	void addBlueprint(T component)
	{
		GameEngine::ECS::Registry &regRef = registry;
		blueprints.emplace_back(
		    [&regRef, component](GameEngine::ECS::Entity e) { regRef.registerComponent(e, component); });
	}

  private:
	std::reference_wrapper<GameEngine::ECS::Registry> registry;
	std::optional<GameEngine::ECS::Entity> currentEntity;

	using ComponentAdder = std::function<void(GameEngine::ECS::Entity)>;
	std::vector<ComponentAdder> blueprints;
};