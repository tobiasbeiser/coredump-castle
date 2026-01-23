#include "KeyParser.hpp"

#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
KeyParser::KeyParser(GameEngine::ECS::Registry &registry, const TileFactory &tileFactory)
    : registry(registry), tileFactory(tileFactory)
{
}
bool KeyParser::canParse(tson::Object &obj) const
{
	return obj.getProperties().hasProperty("key");
}
void KeyParser::parse(tson::Object &obj, const glm::vec2 &levelPos, const int layerIndex) const
{
	const auto entity = tileFactory.createFromObject(obj, levelPos, layerIndex);
	if (!entity.has_value()) {
		return;
	}
	const auto key = obj.get<int>("key");
	registry.registerComponent(entity.value(), KeyComponent{key});
	registry.registerComponent(entity.value(), InteractionComponent{0, 0, static_cast<float>(obj.getSize().x),
	                                                                static_cast<float>(obj.getSize().y)});
}