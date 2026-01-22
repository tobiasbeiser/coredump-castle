#include "SavePointParser.hpp"

#include "Engine/ECS/Components/Core/SavePointComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
SavePointParser::SavePointParser(GameEngine::ECS::Registry &registry, const TileFactory &tileFactory)
    : registry(registry), tileFactory(tileFactory)
{
}

bool SavePointParser::canParse(tson::Object &obj) const
{
	auto tile = tileFactory.getTileFromObject(obj);
	if (!tile.has_value()) {
		return false;
	}
	const auto savePoint = tile.value().getProp("savePoint");
	return savePoint && savePoint->getValue<bool>();
}
void SavePointParser::parse(tson::Object &obj, const glm::vec2 &levelPos, const int layerIndex) const
{
	const auto entity = tileFactory.createFromObject(obj, levelPos, layerIndex);
	if (!entity.has_value()) {
		return;
	}
	registry.registerComponent(entity.value(), SavePointComponent{});
	registry.registerComponent(entity.value(), InteractionComponent{0, 0, static_cast<float>(obj.getSize().x),
	                                                                static_cast<float>(obj.getSize().y)});
}