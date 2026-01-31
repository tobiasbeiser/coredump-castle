#pragma once
#include <SDL3/SDL_rect.h>
struct MeleeAttackComponent {
	SDL_FRect weaponHitBox{0, 0, 0, 0};
	float attackRange{50.0f};
};