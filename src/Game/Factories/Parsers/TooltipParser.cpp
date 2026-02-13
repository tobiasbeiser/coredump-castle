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
	const auto enabledProp = obj.getProp("enabled");
	return tooltipProp && tooltipProp->getType() == tson::Type::String && enabledProp
	       && enabledProp->getType() == tson::Type::Boolean;
}
void TooltipParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	const auto tooltipProp = obj.getProp("tooltip");
	const std::string tooltipText = getTooltipText(tooltipProp->getValue<std::string>());

	const tson::Vector2 pos = obj.getPosition();
	const SDL_FRect tooltipRect = {pos.x + levelPos.x, pos.y + levelPos.y, static_cast<float>(obj.getSize().x),
	                               static_cast<float>(obj.getSize().y)};

	const auto enabledProp = obj.getProp("enabled");
	const bool isEnabled = enabledProp->getValue<bool>();

	const auto entity = registry.create();
	registry.registerComponent(entity, TooltipComponent{.text = tooltipText, .box = tooltipRect, .enabled = isEnabled});
}

std::string TooltipParser::getTooltipText(const std::string &name) const
{
	if (name == "save") {
		return "This is a save point. Press " + inputManager.getHumanReadableKey(Key::Interact)
		       + " to save your progress.";
	}

	if (name == "movement") {
		return "Welcome to Coredump Castle!\n"
		       "The C++ Wizard has been corrupted by ancient C forces,\n"
		       "leaving his magic unsafe and shattering the castle's core.\n"
		       "Fight your way through the castle,\n"
		       "confront the Wizard, and restore the core!\n\n"
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

	if (name == "map") {
		return "Press " + inputManager.getHumanReadableKey(Key::ToggleMap)
		       + " to toggle the map on and off.\n"
		         "Use the the Mousewheel to zoom in and out.";
	}

	if (name == "pause") {
		return "Press " + inputManager.getHumanReadableKey(Key::Escape) + " to toggle the pause menu on and off.";
	}

	if (name == "doublejump") {
		return "Double jump unlocked.\n"
		       "Press "
		       + inputManager.getHumanReadableKey(Key::Up) + " while in the air, to extend your jump.\n";
	}

	return "";
}
