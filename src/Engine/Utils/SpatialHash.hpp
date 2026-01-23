#pragma once

#include "Engine/ECS/Entity.hpp"
#include <SDL3/SDL_rect.h>

#include <cmath>
#include <functional>
#include <unordered_map>
#include <vector>

namespace GameEngine::Utils {

struct CellHash {
	std::size_t operator()(const std::pair<int, int> &p) const
	{
		return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
	}
};

enum class ColliderType { Normal, Hazard };

class SpatialHash {
  public:
	struct Entry {
		ECS::Entity entity{};
		SDL_FRect box{};
		ColliderType type{ColliderType::Normal};
	};

	explicit SpatialHash(const float cellSize = 128.0f) : mCellSize(cellSize) {}

	void insert(const ECS::Entity entity, const SDL_FRect &box, ColliderType type = ColliderType::Normal)
	{
		forEachOverlappingCell(box,
		                       [&](int cellX, int cellY) { mCells[{cellX, cellY}].push_back({entity, box, type}); });
	}

	template <typename Callback>
	void query(const SDL_FRect &queryBox, const Callback &callback) const
	{

		static std::vector<ECS::Entity> checkedEntities;
		checkedEntities.clear();

		checkedEntities.reserve(16);

		forEachOverlappingCell(queryBox, [&](int cellX, int cellY) {
			const auto it = mCells.find({cellX, cellY});
			if (it == mCells.end())
				return;

			for (const auto &entry : it->second) {
				// OPTIMIZATION: Check for duplicates
				bool alreadyChecked = false;
				for (const auto &checked : checkedEntities) {
					if (checked == entry.entity) {
						alreadyChecked = true;
						break;
					}
				}

				if (!alreadyChecked) {
					callback(entry.entity, entry.box, entry.type);
					checkedEntities.push_back(entry.entity);
				}
			}
		});
	}

	template <typename Callback>
	void queryExcluding(const SDL_FRect &queryBox, const ECS::Entity excludeEntity, const Callback &callback) const
	{
		query(queryBox, [&](const ECS::Entity entity, const SDL_FRect &box, const ColliderType type) {
			if (entity != excludeEntity) {
				callback(entity, box, type);
			}
		});
	}

	void clear() { mCells.clear(); }

  private:
	float mCellSize;
	// CHANGED: Unordered map for O(1) lookups
	std::unordered_map<std::pair<int, int>, std::vector<Entry>, CellHash> mCells;

	[[nodiscard]] int toCell(const float position) const { return static_cast<int>(std::floor(position / mCellSize)); }

	template <typename Fn>
	void forEachOverlappingCell(const SDL_FRect &box, const Fn &fn) const
	{
		constexpr float eps = 0.001f;
		const int minCellX = toCell(box.x);
		const int minCellY = toCell(box.y);
		const int maxCellX = toCell(box.x + box.w - eps);
		const int maxCellY = toCell(box.y + box.h - eps);

		for (int cy = minCellY; cy <= maxCellY; ++cy) {
			for (int cx = minCellX; cx <= maxCellX; ++cx) {
				fn(cx, cy);
			}
		}
	}
};

} // namespace GameEngine::Utils