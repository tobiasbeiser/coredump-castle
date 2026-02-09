#include "Game/Factories/OverlayFactory.hpp"

#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Render/HealthBarComponent.hpp"
#include "Engine/ECS/Components/Render/HudOwnerComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/UiElementTag.hpp"
#include "Engine/ECS/Registry.hpp"

#include <unordered_map>

namespace {
HudOwnerType resolveOwner(const std::string &str)
{
	if (str == "Player")
		return HudOwnerType::Player;
	if (str == "Boss")
		return HudOwnerType::Boss;
	return HudOwnerType::None;
}
} // namespace

OverlayFactory::OverlayFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
    : registry(registry), animationFactory(textureManager), tileFactory(registry, textureManager, animationFactory)
{
}

void OverlayFactory::createOverlay(const std::filesystem::path &path)
{
	tson::Tileson tileson;
	const std::unique_ptr<tson::Map> map = tileson.parse(path);

	if (map->getStatus() != tson::ParseStatus::OK)
		return;
	animationFactory.setMap(*map.get());
	tileFactory.setMap(*map.get());

	int layerIndex = 1000;

	for (tson::Layer &layer : map->getLayers()) {
		if (layer.getType() == tson::LayerType::ObjectGroup) {
			loadObjects(layer, layerIndex);
		}
		layerIndex++;
	}
}

void OverlayFactory::loadObjects(tson::Layer &layer, const int &layerIndex) const
{
	constexpr glm::vec2 screenOrigin = {0.0f, 0.0f};
	for (auto &obj : layer.getObjects()) {

		const auto entity = tileFactory.createFromObject(obj, screenOrigin, layerIndex);
		if (!entity.has_value()) {
			continue;
		}

		registry.registerComponent(entity.value(), UiElementTag{});

		if (obj.getProperties().hasProperty("owner")) {
			const std::string ownerStr = obj.getProp("owner")->getValue<std::string>();
			const auto owner = resolveOwner(ownerStr);

			registry.registerComponent(
			    entity.value(), HudOwnerComponent{.type = owner, .trackedEntity = GameEngine::ECS::invalidEntity});
		}

		if (obj.getProperties().hasProperty("type") && obj.getProp("type")->getValue<std::string>() == "Bar") {

			const auto &sprite = registry.get<SpriteComponent>(entity.value());
			registry.registerComponent(entity.value(),
			                           HealthBarComponent{.originalWidth = static_cast<float>(sprite.tileSize.x)});
		}
	}
}