#include "EffectFactory.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Render/EffectComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/Graphics/Animation.hpp"

using namespace std::chrono_literals;

EffectFactory::EffectFactory(GameEngine::ECS::Registry &registry, TextureManager &texture_manager)
    : registry(registry), textureManager(texture_manager)
{
	loadEffect({EffectType::PlayerSwoosh, "PlayerAttackSwoosh.png", 6, 200ms, glm::ivec2(192, 192), glm::ivec2(9, 0),
	            glm::vec2(0.4f, 0.7f), glm::vec2(10.0f, 35.0f), false});

	loadEffect({EffectType::IsHit, "HitEffect.png", 7, 200ms, glm::ivec2(128, 128), glm::ivec2(0, 0),
	            glm::vec2(0.5f, 0.5f), glm::vec2(0.0f, 0.0f), true});
}

void EffectFactory::createEffect(const EffectType type, const glm::vec2 &position, const bool flipX) const
{
	const auto &data = effects.at(type);
	const auto entity = registry.create();

	const auto frameSize = data.frames[0].tileSize;
	const glm::vec2 worldSize = {frameSize.x * data.scale.x, frameSize.y * data.scale.y};

	const glm::vec2 topLeft = position - (worldSize * 0.5f);

	registry.registerComponent<TransformComponent>(entity, {.scale = data.scale});
	registry.registerComponent<PositionComponent>(entity, {topLeft});
	registry.registerComponent<SpriteComponent>(entity, {data.frames.front().texture});

	registry.registerComponent<RenderComponent>(entity, {defaultLayer, flipX});

	auto effectComponent = EffectComponent(data.frames, data.animation);
	effectComponent.animation.start();
	registry.registerComponent<EffectComponent>(entity, effectComponent);
}
void EffectFactory::createIsHitEffect(GameEngine::ECS::Entity attacker, GameEngine::ECS::Entity victim) const
{
	if (registry.has<PlayerTag>(victim)) {
		return;
	}

	const auto &victimHitBox = registry.get<HitBoxComponent>(victim);

	const auto attackerCenter = getEntityCenter(attacker);

	const auto victimCenter = getEntityCenter(victim);
	const bool attackerIsLeftOfVictim = attackerCenter.x < victimCenter.x;

	const auto &data = effects.at(EffectType::IsHit);

	const float dirMult = attackerIsLeftOfVictim ? -1.0f : 1.0f;
	const glm::vec2 finalOffset = {victimHitBox.bodyHitBox.w / 2 + data.offset.x * dirMult, data.offset.y};

	createEffect(EffectType::IsHit, victimCenter + finalOffset, !attackerIsLeftOfVictim);
}

void EffectFactory::createEffectOnEntity(const GameEngine::ECS::Entity entity, const EffectType type) const
{
	if (!effects.contains(type))
		return;

	const auto &data = effects.at(type);
	const auto center = getEntityCenter(entity);

	bool isTargetFlipped = false;

	if (registry.has<RenderComponent>(entity)) {
		const auto &render = registry.get<RenderComponent>(entity);
		isTargetFlipped = render.flipX;
	}

	const float dirMult = isTargetFlipped ? -1.0f : 1.0f;
	const glm::vec2 finalOffset = {data.offset.x * dirMult, data.offset.y};

	createEffect(type, center + finalOffset, isTargetFlipped);
}

void EffectFactory::loadEffect(const LoadEffectData &data)
{
	SDL_Texture &texture = textureManager.getTexture(basePath / data.fileName);
	const Animation animation(data.frameCount, data.duration, false);
	std::vector<AnimationFrame> frames;

	for (int i = 0; i < data.frameCount; i++) {
		glm::ivec2 pos;
		if (data.vertical)
			pos = glm::ivec2(0, i * data.tileSize.y) + data.tilesetOffset * data.tileSize;
		else
			pos = glm::ivec2(i * data.tileSize.x, 0) + data.tilesetOffset * data.tileSize;

		frames.push_back({&texture, data.tileSize, pos});
	}

	effects[data.type] = {frames, animation, data.scale, data.offset};
}

glm::vec2 EffectFactory::getEntityCenter(const GameEngine::ECS::Entity entity) const
{
	const auto &[position, prev] = registry.get<PositionComponent>(entity);
	const auto &sprite = registry.get<SpriteComponent>(entity);

	glm::vec2 scale(1.0f);
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	return position + glm::vec2(sprite.tileSize.x * scale.x * 0.5f, sprite.tileSize.y * scale.y * 0.5f);
}