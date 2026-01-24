#include "HitBoxUtils.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"

SDL_FRect GameEngine::Utils::HitBoxUtils::getWeaponHitBoxRect(const glm::vec2 &pos, const HitBoxComponent &hitBox,
                                                              const SDL_FRect &hurtBox, const glm::vec2 *scale,
                                                              const bool facingLeft)
{
	const auto actualScale = scale ? *scale : glm::vec2{1.0f, 1.0f};

	const auto &[bodyHitBox, weaponHitBox] = hitBox;

	SDL_FRect result = {0, 0, weaponHitBox.w * actualScale.x, weaponHitBox.h * actualScale.y};
	if (facingLeft) {
		result.x = pos.x + hurtBox.x * actualScale.x - weaponHitBox.w * actualScale.x;
	} else {
		result.x = pos.x + hurtBox.x * actualScale.x + hurtBox.w * actualScale.x;
	}
	result.y = pos.y + hurtBox.y * actualScale.y + weaponHitBox.y * actualScale.y;

	return result;
}
SDL_FRect GameEngine::Utils::HitBoxUtils::getHitBox(const glm::vec2 &pos, const SDL_FRect &hitbox,
                                                    const glm::vec2 *scale)
{
	const auto actualScale = scale ? *scale : glm::vec2{1.0f, 1.0f};
	return {pos.x + hitbox.x * actualScale.x, pos.y + hitbox.y * actualScale.y, hitbox.w * actualScale.x,
	        hitbox.h * actualScale.y};
}