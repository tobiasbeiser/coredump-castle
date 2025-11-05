#pragma once
#include "ComponentStorage.hpp"
#include "IStorage.hpp"
#include "StorageWrapper.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace GameEngine::ECS {
template <typename... Components>
class View;

class Registry {
	std::unordered_map<std::type_index, std::unique_ptr<IStorage>> storagePools;

  public:
	template <typename T>
	ComponentStorage<T> &getStorage()
	{
		static const std::type_index id = std::type_index(typeid(T)); // cached once
		auto it = storagePools.find(id);
		if (it == storagePools.end()) {
			it = storagePools.emplace(id, std::make_unique<StorageWrapper<T>>()).first;
		}
		return static_cast<StorageWrapper<T> *>(it->second.get())->storage;
	}

	void clearAll() { storagePools.clear(); }

	int size() const { return next; }

	Entity create()
	{
		return next++;
	}

	template <typename T>
	void registerComponent(Entity entity, T component)
	{
		getStorage<T>().addComponent(entity, std::move(component));
	}

	template <typename T>
	bool has(Entity entity)
	{
		if (!storagePools.count(std::type_index(typeid(T))))
			return false;
		return getStorage<T>().containsComponent(entity);
	}

	template <typename T>
	void remove(Entity entity)
	{
		getStorage<T>().removeComponent(entity);
	}

	template <typename T>
	T &get(Entity entity)
	{
		return getStorage<T>().getComponent(entity);
	}

	template <typename... Components>
	View<Components...> view()
	{
		return View<Components...>(*this);
	}

  private:
	Entity next = 0;
};
} // namespace GameEngine::ECS