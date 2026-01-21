#include "EffectFactory.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Render/EffectComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/Graphics/Animation.hpp"

EffectFactory::EffectFactory(GameEngine::ECS::Registry &registry, TextureManager &texture_manager)
    : registry(registry), textureManager(texture_manager)
{
	loadAnimation(EffectType::PlayerSwoosh, "PlayerAttackSwoosh.png", 5, std::chrono::milliseconds(200),
	              glm::ivec2(192, 192), glm::ivec2(11, 0));
}
void EffectFactory::createEffect(const EffectType type, const glm::vec2 &position, int layer, bool flipX) const
{
	const auto entity = registry.create();
	constexpr glm::vec2 scale{0.4f, 0.7f};
	const auto frameSize = frames.at(type).at(0).tileSize;

	const glm::vec2 scaledHalfSize{frameSize.x * scale.x * 0.5f, frameSize.y * scale.y * 0.5f};
	registry.registerComponent<TransformComponent>(entity, {.scale = scale});
	registry.registerComponent<PositionComponent>(entity, {position - scaledHalfSize});
	registry.registerComponent<SpriteComponent>(entity, {frames.at(type).front().texture});
	registry.registerComponent<RenderComponent>(entity, {100, flipX});

	auto effectComponent = EffectComponent(frames.at(type), animations.at(type));
	effectComponent.animation.start();
	registry.registerComponent<EffectComponent>(entity, effectComponent);
}

void EffectFactory::createAttackEffect(const GameEngine::ECS::Entity entity) const
{
	if (!registry.has<PlayerTag>(entity))
		return;
	const auto center = getEntityCenter(entity);
	const bool flipX = registry.has<RenderComponent>(entity) ? registry.get<RenderComponent>(entity).flipX : false;
	const int layer = registry.has<RenderComponent>(entity) ? registry.get<RenderComponent>(entity).layer + 1 : 101;
	const float offsetX = flipX ? -10.0f : 10.0f;
	createEffect(EffectType::PlayerSwoosh, center + glm::vec2(offsetX, 35.0f), layer, flipX);
}

void EffectFactory::loadAnimation(const EffectType type, const std::string &fileName, int frameCount,
                                  const std::chrono::duration<float> &duration, const glm::ivec2 &tileSize,
                                  const glm::ivec2 &tilesetOffset)
{
	SDL_Texture &playerSwooshTexture = textureManager.getTexture(basePath / fileName);

	Animation playerSwooshAnimation(frameCount, duration, false);
	std::vector<AnimationFrame> playerAnimationFrames{};
	for (int i = 0; i < frameCount; i++) {
		playerAnimationFrames.push_back(
		    AnimationFrame{.texture = &playerSwooshTexture,
		                   .tileSize = tileSize,
		                   .tileMapPosition = glm::ivec2(i * tileSize.x, 0) + tilesetOffset * tileSize});
	}

	animations.emplace(type, playerSwooshAnimation);
	frames.emplace(type, playerAnimationFrames);
}

glm::vec2 EffectFactory::getEntityCenter(const GameEngine::ECS::Entity entity) const
{
	const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
	const auto &sprite = registry.get<SpriteComponent>(entity);

	glm::vec2 scale(1.0f);
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	return position + glm::vec2(sprite.tileSize.x * scale.x * 0.5f, sprite.tileSize.y * scale.y * 0.5f);
}
