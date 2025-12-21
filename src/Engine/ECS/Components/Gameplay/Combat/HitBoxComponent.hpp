#pragma once
#include <SDL3/SDL_rect.h>

struct HitBoxComponent {
	SDL_FRect hitbox{0, 0, 0, 0};
	/// If true: hitbox is the entity's body (contact damage, always active, no offset)
	/// If false: hitbox is a weapon (requires attack state, offset based on facing direction)
	bool isBodyHitBox{false};
};