#pragma once
#include "ComponentStorage.hpp"
#include "IStorage.hpp"

namespace GameEngine { namespace ECS {

template <typename T>
class StorageWrapper : public IStorage {
  public:
	ComponentStorage<T> storage;
};
} } // namespace GameEngine::ECS