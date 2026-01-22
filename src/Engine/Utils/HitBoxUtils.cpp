#include "HitBoxUtils.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Registry.hpp"

SDL_FRect GameEngine::Utils::HitBoxUtils::getWeaponHitBoxRect(ECS::Registry &registry, const ECS::Entity entity)
{
	if (!registry.has<HitBoxComponent>(entity) || !registry.has<PositionComponent>(entity)) {
		return {0, 0, 0, 0};
	}

	const auto &[bodyHitBox, weaponHitBox] = registry.get<HitBoxComponent>(entity);
	const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);

	bool facingLeft = false;
	if (registry.has<RenderComponent>(entity)) {
		facingLeft = registry.get<RenderComponent>(entity).flipX;
	}

	glm::vec2 scale = {1.0f, 1.0f};
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	if (!registry.has<HurtBoxComponent>(entity)) {
		return {0, 0, 0, 0};
	}
	const auto &[hurtBox] = registry.get<HurtBoxComponent>(entity);

	SDL_FRect result = {0, 0, weaponHitBox.w * scale.x, weaponHitBox.h * scale.y};
	if (facingLeft) {
		result.x = position.x + hurtBox.x * scale.x - weaponHitBox.w * scale.x;
	} else {
		result.x = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
	}
	result.y = position.y + hurtBox.y * scale.y + weaponHitBox.y * scale.y;

	return result;
}
SDL_FRect GameEngine::Utils::HitBoxUtils::getHurtBoxRect(ECS::Registry &registry, const ECS::Entity entity)
{

	if (!registry.has<HurtBoxComponent>(entity) || !registry.has<PositionComponent>(entity)) {
		return {0, 0, 0, 0};
	}

	const auto &[hurtBox] = registry.get<HurtBoxComponent>(entity);
	const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);

	glm::vec2 scale = {1.0f, 1.0f};
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	return {position.x + hurtBox.x * scale.x, position.y + hurtBox.y * scale.y, hurtBox.w * scale.x,
	        hurtBox.h * scale.y};
}