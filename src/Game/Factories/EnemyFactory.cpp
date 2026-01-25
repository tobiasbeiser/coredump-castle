#include "EnemyFactory.hpp"
#include "AnimationFactory.hpp"
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include <cctype>
#include <tileson.hpp>

namespace {
struct PatrolRect {
	glm::vec2 min;
	glm::vec2 max;
};
float getFloatProp(tson::Object &obj, const std::string &name, const float defaultValue)
{
	const auto prop = obj.getProp(name);
	if (prop == nullptr) {
		return defaultValue;
	}
	if (prop->getType() != tson::Type::Float) {
		return defaultValue;
	}
	return prop->getValue<float>();
}

int getIntProp(tson::Object &obj, const std::string &name, const int defaultValue)
{
	const auto prop = obj.getProp(name);
	if (prop == nullptr) {
		return defaultValue;
	}
	if (prop->getType() != tson::Type::Int) {
		return defaultValue;
	}
	return prop->getValue<int>();
}

bool getBoolProp(tson::Object &obj, const std::string &name)
{
	const auto prop = obj.getProp(name);
	if (prop == nullptr) {
		return false;
	}
	if (prop->getType() != tson::Type::Boolean) {
		return false;
	}
	return prop->getValue<bool>();
}

std::optional<std::string> getStringProp(tson::Object &obj, const std::string &name, const std::string &defaultValue)
{
	const auto prop = obj.getProp(name);
	if (!prop) {
		return defaultValue.empty() ? std::nullopt : std::optional{defaultValue};
	}
	// Accept both String and File types
	if (prop->getType() != tson::Type::String && prop->getType() != tson::Type::File) {
		return defaultValue.empty() ? std::nullopt : std::optional{defaultValue};
	}

	// For File type, get the filesystem path and convert to string
	if (prop->getType() == tson::Type::File) {
		return prop->getValue<std::filesystem::path>().string();
	}

	return prop->getValue<std::string>();
}

AbilityType parseAbilityType(const std::optional<std::string> &value)
{
	if (!value.has_value()) {
		return AbilityType::None;
	}
	if (value->empty()) {
		return AbilityType::None;
	}
	std::string normalized;
	normalized.reserve(value->size());
	for (const char ch : *value) {
		if (std::isalnum(static_cast<unsigned char>(ch))) {
			normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
		}
	}
	if (normalized == "none") {
		return AbilityType::None;
	}
	if (normalized == "doublejump") {
		return AbilityType::DoubleJump;
	}
	if (normalized == "slide") {
		return AbilityType::Slide;
	}
	if (normalized == "dash") {
		return AbilityType::Dash;
	}
	return AbilityType::None;
}

AbilityType getAbilityProp(tson::Object &obj, const std::string &name, const AbilityType defaultValue)
{
	const auto prop = obj.getProp(name);
	if (prop == nullptr) {
		return defaultValue;
	}
	if (prop->getType() == tson::Type::Enum) {
		const auto enumValue = prop->getValue<tson::EnumValue>();
		return parseAbilityType(enumValue.getValueName());
	}
	if (prop->getType() == tson::Type::String || prop->getType() == tson::Type::File) {
		return parseAbilityType(getStringProp(obj, name, ""));
	}
	return defaultValue;
}

SDL_FRect parseCollisionBox(const std::string &value)
{
	float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
	std::istringstream iss(value);
	char c1 = 0, c2 = 0, c3 = 0;
	if (iss >> x >> c1 >> y >> c2 >> w >> c3 >> h && c1 == ',' && c2 == ',' && c3 == ',') {
		return {x, y, w, h};
	}
	return {0.f, 0.f, 0.f, 0.f};
}

std::optional<PatrolRect> findPatrolRect(tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map)
{
	auto *pathLayer = map.getLayer("Enemies");
	if (!pathLayer) {
		return std::nullopt;
	}

	const auto areaProp = enemyObj.getProp("area");
	if (areaProp == nullptr || areaProp->getType() != tson::Type::Object) {
		return std::nullopt;
	}
	const auto areaID = areaProp->getValue<uint32_t>();
	for (const auto &obj : pathLayer->getObjects()) {
		if (obj.getId() == areaID) {
			const tson::Vector2 pos = obj.getPosition();
			const tson::Vector2 size = obj.getSize();
			return PatrolRect{
			    .min = glm::vec2(pos.x, pos.y) + levelPos,
			    .max = glm::vec2(pos.x + size.x, pos.y + size.y) + levelPos,
			};
		}
	}

	return std::nullopt;
}
} // namespace

GameEngine::ECS::EnemyFactory::EnemyFactory(Registry &registry, AnimationFactory &animationFactory)
    : registry(registry), animationFactory(animationFactory)
{
}

void GameEngine::ECS::EnemyFactory::setMap(tson::Map &tsonMap)
{
	map = &tsonMap;
}

GameEngine::ECS::Entity GameEngine::ECS::EnemyFactory::createEnemy(tson::Object &enemyObj, const glm::vec2 &levelPos,
                                                                   const int layer) const
{
	if (!map) {
		throw std::runtime_error("Map is null");
	}

	const auto entity = registry.create();
	registry.registerComponent(entity, EnemyTag{});

	const auto scale = getFloatProp(enemyObj, "scale", 1.0f);
	const auto maxSpeedX = getFloatProp(enemyObj, "maxSpeedX", 50.0f);
	const auto chaseSpeed = getFloatProp(enemyObj, "chaseSpeed", 0.0f);
	const auto canPatrol = getBoolProp(enemyObj, "canPatrol");
	const auto collisionStr = getStringProp(enemyObj, "collisionBox", "0,0,0,0");
	const auto hurtBoxStr = getStringProp(enemyObj, "hurtBox", "");
	const auto touchDamageBoxStr = getStringProp(enemyObj, "touchDamageBox", "");
	const auto hitBoxStr = getStringProp(enemyObj, "hitBox", "");
	const auto maxHealth = getIntProp(enemyObj, "maxHealth", 100);
	const auto attackCooldown = getFloatProp(enemyObj, "attackCooldown", 0.5);
	const auto reactionDelay = getFloatProp(enemyObj, "reactionDelay", 0.5);
	const auto isRangedEnemy = getBoolProp(enemyObj, "isRangedEnemy");
	const auto projectileSpeed = getFloatProp(enemyObj, "projectileSpeed", 300.0f);
	const auto detectionWidthRange = getFloatProp(enemyObj, "detectionWidthRange", 300.0f);
	const auto detectionHeightRange = getFloatProp(enemyObj, "detectionHeightRange", 50.0f);
	const auto windUpLimit = getFloatProp(enemyObj, "windUpLimit", 0.0f);
	const auto flipXOffset = getBoolProp(enemyObj, "flipXOffset");
	const auto poise = getFloatProp(enemyObj, "poise", 0.0f);
	const auto recoveryTime = getFloatProp(enemyObj, "recoveryTime", 2.0f);
	const auto isBoss = getBoolProp(enemyObj, "isBoss");
	const auto bossIdProp = getIntProp(enemyObj, "bossId", -1);
	const auto unlockAbility = getAbilityProp(enemyObj, "unlockAbility", AbilityType::None);
	const auto chargeChance = getFloatProp(enemyObj, "chargeChance", 0.25f);
	const auto chargeDuration = getFloatProp(enemyObj, "chargeDuration", 0.75f);
	const auto chargePauseDuration = getFloatProp(enemyObj, "chargePauseDuration", 1.0f);
	const auto chargeSpeedMultiplier = getFloatProp(enemyObj, "chargeSpeedMultiplier", 1.5f);
	const auto chargePassThroughFactor = getFloatProp(enemyObj, "chargePassThroughFactor", 0.35f);

	glm::vec2 projectileOffset = {0.0f, 0.0f};
	if (const auto offsetStr = getStringProp(enemyObj, "projectileOffset", "")) {
		const auto parsed = parseCollisionBox(offsetStr.value());
		projectileOffset = {parsed.x, parsed.y};
	}

	const tson::Vector2 pos = enemyObj.getPosition();
	const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

	registry.registerComponent(entity, PositionComponent{.position = spawn});
	registry.registerComponent(entity, VelocityComponent{.velocity = {0.0f, 0.0f}});
	registry.registerComponent(entity, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
	registry.registerComponent(entity, GravityComponent{});
	registry.registerComponent(entity, GroundedComponent{});
	registry.registerComponent(entity, RenderComponent{.layer = layer, .flipX = false, .flipXOffset = flipXOffset});
	registry.registerComponent(entity, TransformComponent{.scale = glm::vec2(scale, scale)});
	registry.registerComponent(entity, StateComponent{});
	using namespace std::chrono_literals;

	registry.registerComponent(entity, InputComponent{});
	registry.registerComponent(entity, MovementComponent{.maxSpeedX = maxSpeedX});
	registry.registerComponent(entity, BlinkComponent{});

	if (isBoss) {
		const int bossId = bossIdProp == -1 ? static_cast<int>(enemyObj.getId()) : bossIdProp;
		registry.registerComponent(entity, BossComponent{.bossId = bossId, .unlockAbility = unlockAbility});
		registry.registerComponent(
		    entity, ChargeComponent{.chargeTimer = Timer{std::chrono::duration<float>(chargeDuration)},
		                            .chargePauseTimer = Timer{std::chrono::duration<float>(chargePauseDuration)},
		                            .chargeChance = chargeChance,
		                            .chargeSpeedMultiplier = chargeSpeedMultiplier,
		                            .chargePassThroughFactor = chargePassThroughFactor});
	}

	SDL_FRect weaponHitBox = {0.f, 0.f, 0.f, 0.f};
	if (hitBoxStr) {
		weaponHitBox = parseCollisionBox(hitBoxStr.value());
	}

	if (collisionStr) {
		const auto collisionBox = parseCollisionBox(collisionStr.value());
		const auto touchDamageBox = touchDamageBoxStr ? parseCollisionBox(touchDamageBoxStr.value()) : collisionBox;
		registry.registerComponent(entity, HitBoxComponent{.bodyHitBox = touchDamageBox, .weaponHitBox = weaponHitBox});
		registry.registerComponent(entity, CollisionComponent{collisionBox});
		registry.registerComponent(entity,
		                           HurtBoxComponent{hurtBoxStr ? parseCollisionBox(hurtBoxStr.value()) : collisionBox});
	}

	if (canPatrol) {
		if (const auto patrolRect = findPatrolRect(enemyObj, levelPos, *map)) {
			registry.registerComponent(entity,
			                           PatrolComponent{.minBounds = patrolRect->min, .maxBounds = patrolRect->max});
		}
		if (chaseSpeed > 0.0f) {
			registry.registerComponent(entity, ChaseComponent{
			                                       .chaseSpeed = chaseSpeed,
			                                       .originalPatrolSpeed = maxSpeedX,
			                                       .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)},
			                                       .detectionHeightRange = detectionHeightRange,
			                                   });
		}
	}
	if (isRangedEnemy) {
		std::string projectileTilesetPath;
		if (const auto tilesetPathOpt = getStringProp(enemyObj, "projectileTileset", "")) {
			const std::filesystem::path basePath = "assets/tiled/tilesets/";
			projectileTilesetPath = (basePath / tilesetPathOpt.value()).string();
		}

		registry.registerComponent(entity, ChaseComponent{
		                                       .chaseSpeed = chaseSpeed,
		                                       .originalPatrolSpeed = maxSpeedX,
		                                       .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)},
		                                       .detectionHeightRange = detectionHeightRange,
		                                       .detectionWidthRange = detectionWidthRange,
		                                   });
		registry.registerComponent(entity, RangedAttackComponent{
		                                       .projectileSpeed = projectileSpeed,
		                                       .projectileOffset = projectileOffset,
		                                   });
		registry.registerComponent(entity, ProjectileComponent{
		                                       .maxRange = 2000.0f,
		                                       .speed = projectileSpeed,
		                                       .spawnPosition = {0.f, 0.f},
		                                       .projectileTilesetPath = projectileTilesetPath,
		                                   });
	}

	animationFactory.loadAnimationsFromTileset(enemyObj, entity);

	using namespace std::chrono_literals;
	std::chrono::duration<float> attackDuration = 100ms;
	std::chrono::duration<float> invulnerabilityTime = 300ms;

	if (auto animComp = registry.tryGet<AnimationComponent>(entity)) {
		if (animComp->get().hasAnimation(AnimationType::Attacking)) {
			attackDuration = animComp->get().getAnimationDuration(AnimationType::Attacking);
		}

		if (animComp->get().hasAnimation(AnimationType::Charge)) {
			attackDuration = animComp->get().getAnimationDuration(AnimationType::Charge);
			if (animComp->get().hasAnimation(AnimationType::Fire)) {
				attackDuration += animComp->get().getAnimationDuration(AnimationType::Fire);
			}
		} else if (animComp->get().hasAnimation(AnimationType::Fire)) {
			attackDuration = animComp->get().getAnimationDuration(AnimationType::Fire);
		}
		if (animComp->get().hasAnimation(AnimationType::Hurting)) {
			invulnerabilityTime = animComp->get().getAnimationDuration(AnimationType::Hurting) + 200ms;
		}
	}

	registry.registerComponent(entity, HealthComponent{maxHealth, maxHealth, invulnerabilityTime});

	if (poise > 0.0f) {
		registry.registerComponent(
		    entity,
		    PoiseComponent{.maxPoise = poise, .recoveryTimer = Timer{std::chrono::duration<float>(recoveryTime)}});
	}

	registry.registerComponent(
	    entity, AttackComponent{.attackTimer = Timer{attackDuration},
	                            .attackCoolDownTimer = Timer{std::chrono::duration<float>(attackCooldown)},
	                            .windUpLimit = windUpLimit,
	                            .activeWindUpLimit = windUpLimit});

	ensureSafeSpawn(entity);

	return entity;
}

void GameEngine::ECS::EnemyFactory::ensureSafeSpawn(const GameEngine::ECS::Entity entity) const
{

	auto position = registry.tryGet<PositionComponent>(entity);
	auto collision = registry.tryGet<CollisionComponent>(entity);

	if (!position || !collision) {
		return;
	}

	const auto transform = registry.tryGet<TransformComponent>(entity);
	const auto scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

	position->get().position.x -=
	    (collision->get().collisionBox.x + (collision->get().collisionBox.w / 2.0f)) * scale.x;
	position->get().position.y -=
	    (collision->get().collisionBox.y + (collision->get().collisionBox.h / 2.0f)) * scale.y;

	int maxAttempts = 100;
	bool hasCollision = true;

	while (hasCollision && maxAttempts > 0) {
		hasCollision = false;
		SDL_FRect enemyRect = {position->get().position.x + collision->get().collisionBox.x * scale.x,
		                       position->get().position.y + collision->get().collisionBox.y * scale.y,
		                       collision->get().collisionBox.w * scale.x, collision->get().collisionBox.h * scale.y};

		for (auto staticColliders = registry.view<PositionComponent, CollisionComponent>()
		                                .exclude<EnemyTag>()
		                                .exclude<PlayerTag>()
		                                .exclude<ProjectileComponent>();
		     const auto staticCollider : staticColliders) {

			const auto &staticColliderPos = staticColliders.get<PositionComponent>(staticCollider);
			const auto &staticColliderCollisionBox =
			    staticColliders.get<CollisionComponent>(staticCollider).collisionBox;
			glm::vec2 staticColliderScale = {1.0f, 1.0f};
			if (const auto staticTransform = registry.tryGet<TransformComponent>(staticCollider)) {
				staticColliderScale = staticTransform->get().scale;
			}

			SDL_FRect staticColliderRect = {
			    staticColliderPos.position.x + staticColliderCollisionBox.x * staticColliderScale.x,
			    staticColliderPos.position.y + staticColliderCollisionBox.y * staticColliderScale.y,
			    staticColliderCollisionBox.w * staticColliderScale.x,
			    staticColliderCollisionBox.h * staticColliderScale.y};

			if (SDL_HasRectIntersectionFloat(&enemyRect, &staticColliderRect)) {
				hasCollision = true;
				position->get().position.y -= 2.0f;
				break;
			}
		}
		maxAttempts--;
	}

	if (maxAttempts == 0) {
		SDL_Log("Warning: Could not safe-spawn enemy entity %u", static_cast<unsigned int>(entity));
	}
}