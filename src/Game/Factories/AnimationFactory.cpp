#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <glm/vec2.hpp>
#include <vector>

AnimationType AnimationFactory::getAnimationType(const std::string &string)
{
	if (string == "Idle") {
		return AnimationType::Idle;
	}
	if (string == "Walking") {
		return AnimationType::Walking;
	}
	if (string == "Attacking") {
		return AnimationType::Attacking;
	}
	if (string == "JumpStart") {
		return AnimationType::JumpStart;
	}
	if (string == "FlyingUp") {
		return AnimationType::FlyingUp;
	}
	if (string == "JumpReload") {
		return AnimationType::JumpReload;
	}
	if (string == "Falling") {
		return AnimationType::Falling;
	}
	if (string == "Landing") {
		return AnimationType::Landing;
	}
	if (string == "Hurting") {
		return AnimationType::Hurting;
	}
	if (string == "Interacting") {
		return AnimationType::Interacting;
	}
	if (string == "Dead") {
		return AnimationType::Dead;
	}
	if (string == "Charge") {
		return AnimationType::Charge;
	}
	if (string == "Fire") {
		return AnimationType::Fire;
	}
	throw std::runtime_error("Unknown state: " + string);
}
AnimationFactory::AnimationData AnimationFactory::loadAnimation(tson::Tile *const &tile) const
{
	auto imageSize = glm::ivec2(tile->getImageSize().x, tile->getImageSize().y);

	const auto &animation = tile->getAnimation();
	const auto tileset = tile->getTileset();

	std::vector<AnimationFrame> frames;
	std::vector<bool> damageFrames;
	std::chrono::duration<float> duration = std::chrono::duration<float>::zero();
	for (const auto frame : animation.getFrames()) {
		duration += std::chrono::duration<float, std::milli>(frame.getDuration());
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

		bool isDamageActive = false;
		auto *tileData = tileset->getTile(frame.getTileId());
		if (tileData) {
			if (const auto prop = tileData->getProp("damageActive")) {
				isDamageActive = prop->getValue<bool>();
			}
		}
		damageFrames.push_back(isDamageActive);

		frames.push_back(animFrame);
	}
	if (const tson::Property *prop = tile->getProp("state"); prop && prop->getType() == tson::Type::Enum) {

		const tson::EnumValue &enumValue = prop->getValue<tson::EnumValue>();
		const AnimationType state = getAnimationType(enumValue.getValueName());

		return {state, frames, damageFrames, duration};
	}

	return {AnimationType::None, frames, damageFrames, duration};

	// if (!registry.has<SpriteComponent>(entity)) {
	// 	registry.registerComponent(entity, SpriteComponent{.texture = nullptr, // to be set by AnimationSystem
	// 	                                                   .tileSize = imageSize,
	// 	                                                   .tileMapPosition = {0, 0}});
	// }
}

std::optional<std::tuple<AnimationComponent, SpriteComponent>>
AnimationFactory::loadAnimationsFromTileset(tson::Object &object) const
{
	if (!map) {
		throw std::runtime_error("Map is null");
	}
	const auto prop = object.getProp("animations");
	if (prop == nullptr || prop->getType() != tson::Type::File) {
		return std::nullopt;
	}

	const auto tilesetPath = basePath / prop->getValue<std::filesystem::path>();

	tson::Tileset tileset;
	tson::Json11 json;
	json.parse(tilesetPath);
	tileset.parse(json, map);

	std::optional<SpriteComponent> spriteComponent;
	AnimationComponent animComponent;

	for (auto &tile : tileset.getTiles()) {
		if (auto anim = tile.getAnimation(); anim.any()) {
			auto [type, frames, damageFrames, duration] = loadAnimation(&tile);

			if (!frames.empty() && !spriteComponent) {
				const auto &frame = frames.back();
				spriteComponent.emplace();
				spriteComponent->texture = frame.texture;
				spriteComponent->tileSize = frame.tileSize;
				spriteComponent->tileMapPosition = {0, 0};
			}

			animComponent.addAnimation(type, frames, damageFrames, duration);
		}
	}

	if (!spriteComponent) {
		return std::nullopt;
	}

	return std::tuple{animComponent, *spriteComponent};
}

// Load animation from a standalone tileset (without map context)
std::optional<std::tuple<AnimationComponent, SpriteComponent>>
AnimationFactory::loadAnimationFromStandaloneTileset(tson::Tileset &tileset, const std::string &tilesetPath) const
{
	if (tileset.getTiles().empty()) {
		return std::nullopt;
	}

	auto &firstTile = tileset.getTiles()[0];
	auto &animation = firstTile.getAnimation();

	if (!animation.any()) {
		return std::nullopt;
	}

	const std::string imageFile = tileset.getImage().string();
	const std::string tilesetDir = tilesetPath.substr(0, tilesetPath.find_last_of("/\\") + 1);
	const std::string texturePath = tilesetDir + imageFile;

	const glm::ivec2 frameSize = {tileset.getTileSize().x, tileset.getTileSize().y};

	const int frameCount = static_cast<int>(animation.getFrames().size());
	int totalDurationMs = 0;
	for (const auto &frame : animation.getFrames()) {
		totalDurationMs += frame.getDuration();
	}
	const float animationDuration = totalDurationMs / 1000.0f;

	auto &texture = textureManager.getTexture(texturePath);

	auto animComp = AnimationComponent{};

	animComp.addAnimationFromSpritesheet(AnimationType::Idle, frameCount,
	                                     std::chrono::duration<float>(animationDuration), &texture, frameSize, {0, 0});
	animComp.setAnimation(AnimationType::Idle);
	auto spriteComp = SpriteComponent{.texture = &texture, .tileSize = frameSize, .tileMapPosition = {0, 0}};

	return std::tuple{animComp, spriteComp};
}

std::optional<std::tuple<AnimationComponent, SpriteComponent>>
AnimationFactory::loadProjectileAnimation(const std::string &tilesetPath) const
{
	tson::Tileset tileset;
	tson::Json11 json;
	if (!json.parse(tilesetPath)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse: %s", tilesetPath.c_str());
		return std::nullopt;
	}
	tileset.parse(json, nullptr);

	return loadAnimationFromStandaloneTileset(tileset, tilesetPath);
}
AnimationComponent AnimationFactory::loadTileAnimation(tson::Tile *const &tile) const
{
	const auto [type, frames, damageFrames, duration] = loadAnimation(tile);
	AnimationComponent animComponent;
	animComponent.addAnimation(type, frames, damageFrames, duration);
	return animComponent;
}