#pragma once
#include "Engine/ECS/Components/Render/AnimationFrame.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Animation.hpp"
#include "Engine/Graphics/TextureManager.hpp"

#include <glm/vec2.hpp>

enum class EffectType {
	PlayerSwoosh,
};

class EffectFactory {
  public:
	EffectFactory(GameEngine::ECS::Registry &registry, TextureManager &texture_manager);

	EffectFactory(const EffectFactory &) = delete;
	EffectFactory &operator=(const EffectFactory &) = delete;
	EffectFactory(EffectFactory &&) = delete;
	EffectFactory &operator=(EffectFactory &&) = delete;
	~EffectFactory() = default;

	void createEffect(EffectType type, const glm::vec2 &position, int layer, bool flipX) const;

	void createAttackEffect(GameEngine::ECS::Entity entity) const;

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	std::unordered_map<EffectType, Animation> animations{};
	std::unordered_map<EffectType, std::vector<AnimationFrame>> frames{};
	void loadAnimation(EffectType type, const std::string &fileName, int frameCount,
	                   const std::chrono::duration<float> &duration, const glm::ivec2 &tileSize,
	                   const glm::ivec2 &tilesetOffset = glm::ivec2(0, 0));

	glm::vec2 getEntityCenter(GameEngine::ECS::Entity entity) const;
	std::filesystem::path basePath{"assets/effects/"};
};
