#pragma once
#include "Engine/ECS/Entity.hpp"

#include <vector>

namespace GameEngine::ECS {
class IStorage {
  public:
	IStorage(const IStorage &) = delete;
	IStorage &operator=(const IStorage &) = delete;
	IStorage(const IStorage &&) = delete;
	IStorage &operator=(const IStorage &&) = delete;
	virtual ~IStorage() = default;

	virtual void remove(Entity entity) = 0;
	[[nodiscard]] virtual size_t getSize() const = 0;
	[[nodiscard]] virtual const std::vector<Entity> &getEntities() const = 0;

  protected:
	IStorage() = default;
};
} // namespace GameEngine::ECS