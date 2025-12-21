#pragma once

#include "Engine/ECS/Entity.hpp"
#include <SDL3/SDL_rect.h>

#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace GameEngine::Utils {

/**
 * A spatial hash grid for efficient spatial queries.
 *
 * Divides 2D space into cells. Objects are inserted into all cells they overlap.
 * Queries return all objects in cells that the query box overlaps.
 *
 * This dramatically speeds up collision detection by only checking nearby objects
 * instead of all objects in the scene.
 */
class SpatialHash {
  public:
	struct Entry {
		ECS::Entity entity{};
		SDL_FRect box{};
	};

	/**
	 * Create a spatial hash with the given cell size.
	 * Larger cells = fewer cells but more objects per cell.
	 * Smaller cells = more cells but fewer objects per cell.
	 * 128 pixels is a good default for tile-based games.
	 */
	explicit SpatialHash(const float cellSize = 128.0f) : mCellSize(cellSize) {}

	/**
	 * Insert an entity's bounding box into the grid.
	 * The entity will be added to all cells that the box overlaps.
	 */
	void insert(const ECS::Entity entity, const SDL_FRect &box)
	{
		forEachOverlappingCell(box, [&](int cellX, int cellY) { mCells[{cellX, cellY}].push_back({entity, box}); });
	}

	/**
	 * Query all entities whose boxes might overlap the given query box.
	 * Calls callback(entity, box) for each candidate.
	 *
	 * Note: Candidates are not guaranteed to actually intersect - this is a
	 * "broad phase" that reduces the number of precise intersection tests needed.
	 */
	template <typename Callback>
	void query(const SDL_FRect &queryBox, const Callback &callback) const
	{
		forEachOverlappingCell(queryBox, [&](int cellX, int cellY) {
			const auto it = mCells.find({cellX, cellY});
			if (it == mCells.end())
				return;

			for (const auto &entry : it->second) {
				callback(entry.entity, entry.box);
			}
		});
	}

	/**
	 * Query, but skip a specific entity (useful when checking an entity
	 * against everything except itself).
	 */
	template <typename Callback>
	void queryExcluding(const SDL_FRect &queryBox, const ECS::Entity excludeEntity, const Callback &callback) const
	{
		query(queryBox, [&](ECS::Entity entity, const SDL_FRect &box) {
			if (entity != excludeEntity) {
				callback(entity, box);
			}
		});
	}

	/** Remove all entries from the grid. */
	void clear() { mCells.clear(); }

  private:
	float mCellSize;
	std::map<std::pair<int, int>, std::vector<Entry>> mCells;

	/** Convert a world position to a cell coordinate. */
	[[nodiscard]] int toCell(const float position) const { return static_cast<int>(std::floor(position / mCellSize)); }

	/**
	 * Call fn(cellX, cellY) for each cell that the box overlaps.
	 */
	template <typename Fn>
	void forEachOverlappingCell(const SDL_FRect &box, const Fn &fn) const
	{
		// Subtract small epsilon from max bounds to avoid including cells
		// that the box just barely touches at the edge
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
