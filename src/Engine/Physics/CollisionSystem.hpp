#pragma once

#include <vector>
#include <memory>

#include "../Game/Entities/IEntity.hpp"
#include "../Game/Level/Level.hpp"

class CollisionSystem {
public:
	void Resolve(std::vector<std::unique_ptr<GameObject>>& objects, float deltaTime) const;
	void ResolveCollisions(SDL_FRect& entityBox, const SDL_FRect& tileBox, std::reference_wrapper<IEntity>& entity) const;
};