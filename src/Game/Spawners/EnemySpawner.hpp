#pragma once
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "tileson.hpp"
#include <functional>

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

	void addSetupStep(std::function<void(GameEngine::ECS::Entity)> step)
	{
		blueprints.emplace_back(step);
	}

  private:
	std::reference_wrapper<GameEngine::ECS::Registry> registry;
	std::optional<GameEngine::ECS::Entity> currentEntity;

	using ComponentAdder = std::function<void(GameEngine::ECS::Entity)>;
	std::vector<ComponentAdder> blueprints;
};