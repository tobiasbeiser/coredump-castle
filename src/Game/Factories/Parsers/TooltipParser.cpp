#include "TooltipParser.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL_rect.h>

TooltipParser::TooltipParser(GameEngine::ECS::Registry &registry, const InputManager &inputManager)
    : registry(registry), inputManager(inputManager)
{
}
bool TooltipParser::canParse(tson::Object &obj) const
{
	const auto tooltipProp = obj.getProp("tooltip");
	return tooltipProp && tooltipProp->getType() == tson::Type::String;
}
void TooltipParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	const auto tooltipProp = obj.getProp("tooltip");
	const std::string tooltipText = getTooltipText(tooltipProp->getValue<std::string>());

	const tson::Vector2 pos = obj.getPosition();
	const SDL_FRect tooltipRect = {pos.x + levelPos.x, pos.y + levelPos.y, static_cast<float>(obj.getSize().x),
	                               static_cast<float>(obj.getSize().y)};

	const auto entity = registry.create();
	registry.registerComponent(entity, TooltipComponent{.text = tooltipText, .box = tooltipRect});
}

std::string TooltipParser::getTooltipText(const std::string &name) const
{
	if (name == "save") {
		return "This is a save point. Press " + inputManager.getHumanReadableKey(Key::Interact)
		       + " to save your progress.";
	}

	if (name == "movement") {
		return "Welcome to Coredump Castle!\n"
		       "The C++ Wizard has been corrupted by ancient C forces, leaving his magic unsafe and shattering the "
		       "castle's core.\n"
		       "Fight your way through the castle, confront the Wizard, and restore the core!\n\n"
		       "Press "
		       + inputManager.getHumanReadableKey(Key::Left) + " and " + inputManager.getHumanReadableKey(Key::Right)
		       + " to move.";
	}

	if (name == "jump") {
		return "Press " + inputManager.getHumanReadableKey(Key::Up) + " to jump.";
	}
	if (name == "combat") {
		return "Press " + inputManager.getHumanReadableKey(Key::Attack) + " to attack.";
	}

	return "";
}
