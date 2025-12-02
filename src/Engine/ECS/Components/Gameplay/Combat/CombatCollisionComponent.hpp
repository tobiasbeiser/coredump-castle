#pragma once
#include "Engine/ECS/Entity.hpp"
#include <SDL3/SDL_rect.h>
#include <optional>

struct CombatCollisionComponent {
	SDL_FRect hitbox{0, 0, 0, 0};         // Offset and size for combat detection
	std::optional<GameEngine::ECS::Entity> collidedWith{}; // Track what entity we collided with
	bool isHit = false;                   // Track if this entity was hit this frame
};
