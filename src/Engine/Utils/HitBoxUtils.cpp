#include "HitBoxUtils.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"

SDL_FRect GameEngine::Utils::HitBoxUtils::getWeaponHitBoxRect(const glm::vec2 &pos, const HitBoxComponent &hitBox,
                                                              const SDL_FRect &hurtBox, const glm::vec2 scale,
                                                              const bool facingLeft)
{

	const auto &[bodyHitBox, weaponHitBox] = hitBox;

	SDL_FRect result = {0, 0, weaponHitBox.w * scale.x, weaponHitBox.h * scale.y};
	if (facingLeft) {
		result.x = pos.x + hurtBox.x * scale.x - weaponHitBox.w * scale.x;
	} else {
		result.x = pos.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
	}
	result.y = pos.y + hurtBox.y * scale.y + weaponHitBox.y * scale.y;

	return result;
}
SDL_FRect GameEngine::Utils::HitBoxUtils::getHitBox(const glm::vec2 &pos, const SDL_FRect &hitbox,
                                                    const glm::vec2 &scale)
{
	return {pos.x + hitbox.x * scale.x, pos.y + hitbox.y * scale.y, hitbox.w * scale.x, hitbox.h * scale.y};
}