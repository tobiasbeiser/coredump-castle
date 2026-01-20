#include "SavePointParser.hpp"

#include "Engine/ECS/Components/Core/SavePointComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
SavePointParser::SavePointParser(GameEngine::ECS::Registry &registry, const TileFactory &tileFactory)
    : registry(registry), tileFactory(tileFactory)
{
}

bool SavePointParser::canParse(tson::Object &obj) const
{
	return obj.getName() == "SavePoint";
}
void SavePointParser::parse(tson::Object &obj, const glm::vec2 &levelPos, const int layerIndex) const
{
	const auto entity = tileFactory.createFromObject(obj, levelPos, layerIndex);
	registry.registerComponent(entity, SavePointComponent{});
	registry.registerComponent(
	    entity, InteractionComponent{0, 0, static_cast<float>(obj.getSize().x), static_cast<float>(obj.getSize().y)});
}