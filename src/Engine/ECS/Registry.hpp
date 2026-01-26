#pragma once
#include "ComponentStorage.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Entity.hpp"
#include "IStorage.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace GameEngine::ECS {
template <typename... Components>
class View;

class Registry {
	std::vector<std::unique_ptr<IStorage>> storagePools;

  public:
	template <typename T>
	ComponentStorage<T> &getStorage()
	{
		const size_t id = ComponentId<T>::value;

		if (id >= storagePools.size()) {
			storagePools.resize(id + 1);
		}

		if (!storagePools[id]) {
			storagePools[id] = std::make_unique<ComponentStorage<T>>();
		}

		return *static_cast<ComponentStorage<T> *>(storagePools[id].get());
	}

	void deleteEntity(const Entity entity) const
	{
		for (const auto &storagePool : storagePools) {
			if (storagePool) {
				storagePool->remove(entity);
			}
		}
	}

	void clearAll() { storagePools.clear(); }

	[[nodiscard]] int size() const { return next; }

	Entity create() { return next++; }

	template <typename T>
	void registerComponent(Entity entity, T component)
	{
		getStorage<T>().insert(entity, std::move(component));
	}

	template <typename... Components>
	bool has(const Entity entity) const
	{
		return ([&]() {
			const size_t id = ComponentId<Components>::value;
			if (id >= storagePools.size() || !storagePools[id])
				return false;
			return static_cast<ComponentStorage<Components> *>(storagePools[id].get())->contains(entity);
		}() && ...);
	}

	template <typename T>
	void remove(Entity entity)
	{
		getStorage<T>().remove(entity);
	}

	template <typename T>
	T &get(Entity entity)
	{
		return getStorage<T>().get(entity);
	}

	template <typename T>
	std::optional<std::reference_wrapper<T>> tryGet(Entity entity)
	{
		const size_t id = ComponentId<T>::value;
		if (id >= storagePools.size() || !storagePools[id]) {
			return std::nullopt;
		}
		auto *storage = static_cast<ComponentStorage<T> *>(storagePools[id].get());

		if (!storage->contains(entity)) {
			return std::nullopt;
		}

		return storage->get(entity);
	}

	template <typename... Components>
	View<Components...> view()
	{
		return View<Components...>(*this);
	}

  private:
	Entity next = 0;

	struct ComponentCounter {
		static inline std::atomic<size_t> counter = 0;
	};

	template <typename T>
	struct ComponentId {
		static inline const size_t value = ComponentCounter::counter++;
	};
};
} // namespace GameEngine::ECS