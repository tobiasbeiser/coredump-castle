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
	ComponentStorage<T> &get_storage()
	{
		const auto id = std::type_index(typeid(T));
		if (!storagePools.count(id)) {
			storagePools[id] = std::make_unique<StorageWrapper<T>>();
		}

		return static_cast<StorageWrapper<T> *>(storagePools[id].get())->storage;
	}

	void clearAll() { storagePools.clear(); }

	int size() const { return next; }

	Entity create()
	{
		return next++;
	}

	template <typename T>
	void register_component(Entity entity, T component)
	{
		get_storage<T>().add_component(entity, std::move(component));
	}

	template <typename T>
	bool has(Entity entity)
	{
		if (!storagePools.count(std::type_index(typeid(T))))
			return false;
		return get_storage<T>().contains_component(entity);
	}

	template <typename T>
	T &get(Entity entity)
	{
		return get_storage<T>().get_component(entity);
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