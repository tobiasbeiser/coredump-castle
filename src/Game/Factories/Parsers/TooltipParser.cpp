#include "TooltipParser.hpp"

#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Registry.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_rect.h>
TooltipParser::TooltipParser(GameEngine::ECS::Registry &registry) : registry(registry) {}
bool TooltipParser::canParse(tson::Object &obj) const
{
	const auto tooltipProp = obj.getProp("tooltip");
	return tooltipProp && tooltipProp->getType() == tson::Type::String;
}
void TooltipParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	const auto tooltipProp = obj.getProp("tooltip");
	const std::string tooltipText = tooltipProp->getValue<std::string>();

	const tson::Vector2 pos = obj.getPosition();
	const SDL_FRect tooltipRect = {pos.x + levelPos.x, pos.y + levelPos.y, static_cast<float>(obj.getSize().x),
	                               static_cast<float>(obj.getSize().y)};

	const auto entity = registry.create();
	registry.registerComponent(entity, TooltipComponent{.text = tooltipText, .box = tooltipRect});
}