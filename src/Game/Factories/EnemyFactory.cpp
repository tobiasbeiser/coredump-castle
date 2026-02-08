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
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/VisionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
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
#include "Game/AI/StateMachine/EnemyStateMachineFactory.hpp"
#include "Game/Spawners/EnemySpawner.hpp"

#include <tileson.hpp>

namespace {
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
} // namespace

GameEngine::ECS::EnemyFactory::EnemyFactory(Registry &registry, AnimationFactory &animationFactory)
    : registry(registry), animationFactory(animationFactory)
{
}

void GameEngine::ECS::EnemyFactory::setMap(tson::Map &tsonMap)
{
	map = &tsonMap;
}

EnemySpawner GameEngine::ECS::EnemyFactory::createEnemy(tson::Object &enemyObj, const glm::vec2 &levelPos,
                                                        const int layer) const
{
	if (!map) {
		throw std::runtime_error("Map is null");
	}

	auto spawner = EnemySpawner(registry);

	spawner.addBlueprint(EnemyTag{});

	const auto scale = getFloatProp(enemyObj, "scale", 1.0f);
	const auto maxSpeedX = getFloatProp(enemyObj, "maxSpeedX", 50.0f);
	const auto chaseSpeed = getFloatProp(enemyObj, "chaseSpeed", 0.0f);
	const auto canPatrol = getBoolProp(enemyObj, "canPatrol");
	const auto collisionStr = getStringProp(enemyObj, "collisionBox", "0,0,0,0");
	const auto hurtBoxStr = getStringProp(enemyObj, "hurtBox", "");
	const auto weaponHitBoxStr = getStringProp(enemyObj, "weaponHitBox", "");
	const auto touchDamageBoxStr = getStringProp(enemyObj, "touchDamageBox", "");
	const auto hitBoxStr = getStringProp(enemyObj, "hitBox", "");
	const auto maxHealth = getIntProp(enemyObj, "maxHealth", 100);
	const auto attackCooldown = getFloatProp(enemyObj, "attackCooldown", 0.5);
	const auto reactionDelay = getFloatProp(enemyObj, "reactionDelay", 0.5);
	const auto isRangedEnemy = getBoolProp(enemyObj, "isRangedEnemy");
	const auto projectileSpeed = getFloatProp(enemyObj, "projectileSpeed", 300.0f);
	const auto windUpLimit = getFloatProp(enemyObj, "windUpLimit", 0.0f);
	const auto flipXOffset = getBoolProp(enemyObj, "flipXOffset");
	const auto poise = getFloatProp(enemyObj, "poise", 0.0f);
	const auto recoveryTime = getFloatProp(enemyObj, "recoveryTime", 2.0f);
	const auto isBoss = getBoolProp(enemyObj, "isBoss");
	const auto bossIdProp = getIntProp(enemyObj, "bossId", -1);
	const auto unlockAbility = getAbilityProp(enemyObj, "unlockAbility", AbilityType::None);
	const auto chargeChance = getFloatProp(enemyObj, "chargeChance", 0.35f);
	const auto chargeDuration = getFloatProp(enemyObj, "chargeDuration", 0.75f);
	const auto chargePauseDuration = getFloatProp(enemyObj, "chargePauseDuration", 1.0f);
	const auto chargeSpeedMultiplier = getFloatProp(enemyObj, "chargeSpeedMultiplier", 1.5f);
	const auto chargeCooldown = getFloatProp(enemyObj, "chargeCooldown", 3.0f);
	const auto detectionRange = getFloatProp(enemyObj, "detectionRange", -1);
	const auto attackRange = getFloatProp(enemyObj, "attackRange", -1);

	glm::vec2 projectileOffset = {0.0f, 0.0f};
	if (const auto offsetStr = getStringProp(enemyObj, "projectileOffset", "")) {
		const auto parsed = parseCollisionBox(offsetStr.value());
		projectileOffset = {parsed.x, parsed.y};
	}

	const tson::Vector2 pos = enemyObj.getPosition();
	const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

	spawner.addBlueprint(PositionComponent{.position = spawn});
	spawner.addBlueprint(VelocityComponent{.velocity = {0.0f, 0.0f}});
	spawner.addBlueprint(AccelerationComponent{.acceleration = {0.0f, 0.0f}});
	spawner.addBlueprint(GravityComponent{});
	spawner.addBlueprint(GroundedComponent{});
	spawner.addBlueprint(RenderComponent{.layer = layer, .flipX = false, .flipXOffset = flipXOffset});
	spawner.addBlueprint(TransformComponent{.scale = glm::vec2(scale, scale)});
	spawner.addBlueprint(StateComponent{});
	using namespace std::chrono_literals;

	spawner.addBlueprint(InputComponent{});
	spawner.addBlueprint(MovementComponent{.maxSpeedX = maxSpeedX});
	spawner.addBlueprint(BlinkComponent{});

	if (isBoss) {
		const int bossId = bossIdProp == -1 ? enemyObj.getId() : bossIdProp;
		spawner.addBlueprint(BossComponent{.bossId = bossId, .unlockAbility = unlockAbility});
		spawner.addBlueprint(ChargeComponent{
		    .chargeCooldown = Timer{std::chrono::duration<float>(chargeCooldown), true},
		    .chargeTimer = Timer{std::chrono::duration<float>(chargeDuration)},
		    .chargePauseTimer = Timer{std::chrono::duration<float>(chargePauseDuration)},
		    .chargeSpeedMultiplier = chargeSpeedMultiplier,
		    .chargeChance = chargeChance,
		});
	}

	using namespace std::chrono_literals;
	std::chrono::duration<float> attackDuration = 100ms;
	std::chrono::duration<float> invulnerabilityTime = 300ms;

	float spriteWidth = 0.0f;

	if (const auto animComp = animationFactory.loadAnimationsFromTileset(enemyObj)) {
		auto &[animationComponent, spriteComponent] = *animComp;

		spawner.addBlueprint(animationComponent);
		spawner.addBlueprint(spriteComponent);

		spriteWidth = static_cast<float>(spriteComponent.tileSize.x);

		if (animationComponent.hasAnimation(AnimationType::Attacking)) {
			attackDuration = animationComponent.getAnimationDuration(AnimationType::Attacking);
		}

		if (animationComponent.hasAnimation(AnimationType::Charge)) {
			attackDuration = animationComponent.getAnimationDuration(AnimationType::Charge);
			if (animationComponent.hasAnimation(AnimationType::Fire)) {
				attackDuration += animationComponent.getAnimationDuration(AnimationType::Fire);
			}
		} else if (animationComponent.hasAnimation(AnimationType::Fire)) {
			attackDuration = animationComponent.getAnimationDuration(AnimationType::Fire);
		}
		if (animationComponent.hasAnimation(AnimationType::Hurting)) {
			invulnerabilityTime = animationComponent.getAnimationDuration(AnimationType::Hurting) + 200ms;
		}
	}

	if (weaponHitBoxStr) {
		const auto weaponHitBox = parseCollisionBox(weaponHitBoxStr.value());

		if (weaponHitBox.w >= 0.f && weaponHitBox.h >= 0.f) {
			spawner.addBlueprint(MeleeAttackComponent{weaponHitBox, attackRange > 0 ? attackRange : weaponHitBox.w});
		}
	}

	if (collisionStr) {
		auto collisionBox = parseCollisionBox(collisionStr.value());
		if (spriteWidth > 0.0f && collisionBox.w > 0.0f) {
			collisionBox.x = (spriteWidth - collisionBox.w) * 0.5f;
		}

		const auto touchDamageBox = touchDamageBoxStr ? parseCollisionBox(touchDamageBoxStr.value()) : collisionBox;
		spawner.addBlueprint(HitBoxComponent{.bodyHitBox = touchDamageBox});
		spawner.addBlueprint(CollisionComponent{collisionBox});
		spawner.addBlueprint(HurtBoxComponent{hurtBoxStr ? parseCollisionBox(hurtBoxStr.value()) : collisionBox});
	}

	if (canPatrol) {
		spawner.addBlueprint(PatrolComponent{.patrolSpeed = maxSpeedX, .patrolCenter = spawn});
	}

	if (chaseSpeed > 0.0f) {
		const float effectiveDetectionRange = detectionRange > 0 ? detectionRange : 200.0f;
		spawner.addBlueprint(ChaseComponent{
		    .chaseSpeed = chaseSpeed,
		    .detectionRange = effectiveDetectionRange,
		    .giveUpRange = effectiveDetectionRange + 50.0f,
		    .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)},
		});
	}
	if (isRangedEnemy) {
		SDL_Log("[EnemyFactory] Creating ranged enemy at (%.1f, %.1f)", spawn.x, spawn.y);

		std::string projectileTilesetPath;
		if (const auto tilesetPathOpt = getStringProp(enemyObj, "projectileTileset", "")) {
			const std::string relPath = tilesetPathOpt.value();
			if (!relPath.empty() && relPath.find(".tsj") != std::string::npos) {
				const std::filesystem::path basePath = "assets/tiled/tilesets/";
				projectileTilesetPath = (basePath / relPath).string();
				SDL_Log("[EnemyFactory] Projectile tileset path: %s", projectileTilesetPath.c_str());
			} else {
				SDL_Log("[EnemyFactory] WARNING: Invalid or empty projectileTileset property for enemy %s: '%s'",
				        enemyObj.getName().c_str(), relPath.c_str());
			}
		}

		const float effectiveDetectionRange = detectionRange > 0 ? detectionRange : 300.0f;
		spawner.addBlueprint(ChaseComponent{
		    .chaseSpeed = chaseSpeed,
		    .detectionRange = effectiveDetectionRange,
		    .giveUpRange = effectiveDetectionRange + 50.0f,
		    .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)},
		});
		if (isBoss) {
			spawner.addBlueprint(VisionComponent{.visionRange = effectiveDetectionRange});
		}
		spawner.addBlueprint(RangedAttackComponent{
		    .projectileSpeed = projectileSpeed,
		    .projectileOffset = projectileOffset,
		    .attackRange = attackRange,
		    .useSpreadShot = isBoss,
		    .spreadCount = 3,
		    .spreadAngle = 12.0f,
		});
		spawner.addBlueprint(ProjectileComponent{
		    .maxRange = 2000.0f,
		    .speed = projectileSpeed,
		    .spawnPosition = {0.f, 0.f},
		    .projectileTilesetPath = projectileTilesetPath,
		});

		auto reg = registry;
		if (isBoss) {
			spawner.addSetupStep(
			    [reg](const Entity entity) { EnemyStateMachineFactory::createArcherBoss(reg, entity); });
		} else {
			spawner.addSetupStep(
			    [reg](const Entity entity) { EnemyStateMachineFactory::createWizardEnemy(reg, entity); });
		}
	} else if (!isBoss && chaseSpeed > 0) {
		auto reg = registry;
		spawner.addSetupStep([reg](const Entity entity) { EnemyStateMachineFactory::createMeleeEnemy(reg, entity); });
	} else if (!isBoss) {
		auto reg = registry;
		spawner.addSetupStep(
		    [reg](const Entity entity) { EnemyStateMachineFactory::createPatrolOnlyEnemy(reg, entity); });
	}

	spawner.addBlueprint(HealthComponent{maxHealth, maxHealth, invulnerabilityTime});

	if (poise > 0.0f) {
		spawner.addBlueprint(
		    PoiseComponent{.maxPoise = poise, .recoveryTimer = Timer{std::chrono::duration<float>(recoveryTime)}});
	}

	spawner.addBlueprint(AttackComponent{.attackTimer = Timer{attackDuration},
	                                     .attackCoolDownTimer = Timer{std::chrono::duration<float>(attackCooldown)},
	                                     .windUpLimit = windUpLimit,
	                                     .activeWindUpLimit = windUpLimit});

	if (detectionRange > 0.0f && !(isBoss && isRangedEnemy)) {
		spawner.addBlueprint(VisionComponent{.visionRange = detectionRange});
	}

	auto reg = registry;
	spawner.addSetupStep([reg](const Entity entity) { ensureSafeSpawn(reg, entity); });
	if (isBoss && !isRangedEnemy) {
		spawner.addSetupStep([reg](const Entity entity) { EnemyStateMachineFactory::createBossEnemy(reg, entity); });
	}
	return spawner;
}

void GameEngine::ECS::EnemyFactory::ensureSafeSpawn(Registry &registry, const Entity entity)
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