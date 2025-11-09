#pragma once
#include "ComponentStorage.hpp"
#include "IStorage.hpp"

namespace GameEngine::ECS {

template <typename T>
class TypedStorage final : public IStorage {
  public:
	ComponentStorage<T> storage;

	void removeComponent(Entity entity) override
	{
		storage.removeComponent(entity);
	}
};
} // namespace GameEngine::ECS