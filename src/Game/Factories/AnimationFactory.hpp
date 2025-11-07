#pragma once
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "tileson.hpp"

#include <glm/vec2.hpp>

class AnimationFactory {
  public:
	AnimationFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager)
	{
	}

	AnimationFactory(const AnimationFactory &) = delete;
	AnimationFactory &operator=(const AnimationFactory &) = delete;
	AnimationFactory(AnimationFactory &&) = delete;
	AnimationFactory &operator=(AnimationFactory &&) = delete;
	~AnimationFactory() = default;

	static State getStateFromString(const std::string &string)
	{
		if (string == "Idle") {
			return State::Idle;
		}
		if (string == "Walking") {
			return State::Walking;
		}
		if (string == "Attacking") {
			return State::Attacking;
		}
		if (string == "Jumping") {
			return State::Jumping;
		}
		if (string == "Falling") {
			return State::Falling;
		}

		throw std::runtime_error("unknown state: " + string);
	}

	void loadAnimation(tson::Tile *const &tile, const GameEngine::ECS::Entity entity) const
	{
		auto imageSize = glm::ivec2(tile->getImageSize().x, tile->getImageSize().y);

		const auto &animation = tile->getAnimation();
		const auto tileset = tile->getTileset();

		if (!registry.has<AnimationComponent>(entity)) {
			registry.registerComponent(entity, AnimationComponent{});
		}

		std::vector<AnimationFrame> frames;
		float duration = 0.0f;
		for (const auto frame : animation.getFrames()) {
			duration += frame.getDuration() / 1000.0f;
			glm::ivec2 tileMapPosition{0, 0};
			std::filesystem::path frameImagePath = basePath;
			if (tileset->getTile(frame.getTileId())->getImage().empty()) {
				const auto correctTile = tileset->getTile(frame.getTileId());
				tileMapPosition = glm::ivec2(correctTile->getDrawingRect().x, correctTile->getDrawingRect().y);
				glm::ivec2 levelTileSize =
				    glm::ivec2(correctTile->getMap()->getTileSize().x, correctTile->getMap()->getTileSize().y);
				imageSize = glm::ivec2(tileset->getTileSize().x, tileset->getTileSize().y);

				tileMapPosition =
				    tileMapPosition / levelTileSize
				    * imageSize; // tileson parses the tile size from the map, but we need the tileset tile size here

				for (const auto &part : tileset->getImage()) {
					if (part != "..") {
						frameImagePath /= part;
					}
				}
			} else {
				frameImagePath /= tileset->getTile(frame.getTileId())->getImage();
			}

			AnimationFrame animFrame{};
			animFrame.texture = &textureManager.getTexture(frameImagePath);
			animFrame.tileMapPosition = tileMapPosition;
			animFrame.tileSize = imageSize;
			frames.push_back(animFrame);
		}
		auto &animComp = registry.get<AnimationComponent>(entity);

		if (const tson::Property *prop = tile->getProp("state"); prop && prop->getType() == tson::Type::Enum) {

			const tson::EnumValue &enumValue = prop->getValue<tson::EnumValue>();
			const State state = getStateFromString(enumValue.getValueName());
			if (!registry.has<StateComponent>(entity)) {
				registry.registerComponent(entity, StateComponent{});
			}
			animComp.addAnimation(state, frames, duration);
		} else {
			int key = 0;
			animComp.animations[key] = Animation(static_cast<int>(frames.size()), duration);
			animComp.frames[key] = frames;
		}

		if (!registry.has<SpriteComponent>(entity)) {
			registry.registerComponent(entity, SpriteComponent{.texture = nullptr, // to be set by AnimationSystem
			                                                   .tileSize = imageSize,
			                                                   .tileMapPosition = {0, 0}});
		}
	}

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
	const std::filesystem::path basePath = "assets/tiled/tilesets/";
};