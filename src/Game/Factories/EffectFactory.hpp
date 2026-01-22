#pragma once
#include "Engine/ECS/Components/Render/AnimationFrame.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Animation.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include <glm/vec2.hpp>
#include <string>

enum class EffectType {
	PlayerSwoosh,
	IsHit,
};

struct EffectData {
	std::vector<AnimationFrame> frames;
	Animation animation;
	glm::vec2 scale{1.0f, 1.0f};
	glm::vec2 offset{0.0f, 0.0f};
};

struct LoadEffectData {
	EffectType type;
	std::string fileName;
	int frameCount;
	std::chrono::duration<float> duration;
	glm::ivec2 tileSize;
	glm::ivec2 tilesetOffset;
	glm::vec2 scale;
	glm::vec2 offset;
	bool vertical = false;
};

class EffectFactory {
  public:
	EffectFactory(GameEngine::ECS::Registry &registry, TextureManager &texture_manager);

	EffectFactory(const EffectFactory &) = delete;
	EffectFactory &operator=(const EffectFactory &) = delete;
	EffectFactory(EffectFactory &&) = delete;
	EffectFactory &operator=(EffectFactory &&) = delete;
	~EffectFactory() = default;

	void createEffect(EffectType type, const glm::vec2 &position, bool flipX) const;
	void createIsHitEffect(GameEngine::ECS::Entity attacker, GameEngine::ECS::Entity victim) const;
	void createEffectOnEntity(GameEngine::ECS::Entity entity, EffectType type) const;

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;

	std::unordered_map<EffectType, EffectData> effects;
	void loadEffect(const LoadEffectData &data);

	glm::vec2 getEntityCenter(GameEngine::ECS::Entity entity) const;
	std::filesystem::path basePath{"assets/effects/"};
	static constexpr int defaultLayer = 100;
};
