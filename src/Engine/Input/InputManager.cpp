#include "InputManager.hpp"
#include "Engine/Debug/DebugSystem.hpp"

#include <ranges>

InputManager::InputManager()
{
	scancodeMapping = {{SDL_SCANCODE_A, Key::Left},     {SDL_SCANCODE_D, Key::Right},    {SDL_SCANCODE_SPACE, Key::Up},
	                   {SDL_SCANCODE_S, Key::Down},     {SDL_SCANCODE_E, Key::Interact}, {SDL_SCANCODE_K, Key::Attack},
	                   {SDL_SCANCODE_M, Key::ToggleMap}, {SDL_SCANCODE_ESCAPE, Key::Escape}};

	for (auto &key : scancodeMapping | std::views::values) {
		previousKeyStates[key] = false;
		currentKeyStates[key] = false;
	}
}

void InputManager::processEvent(const SDL_KeyboardEvent *event)
{
	if (scancodeMapping.contains(event->scancode)) {
		const Key key = scancodeMapping.at(event->scancode);
		currentKeyStates[key] = event->down;
	}
}

void InputManager::update()
{
	previousKeyStates = currentKeyStates;
}

bool InputManager::isKeyDown(const Key key) const
{
	return currentKeyStates.at(key);
}

bool InputManager::isKeyPressed(const Key key) const
{
	return currentKeyStates.at(key) && !previousKeyStates.at(key);
}

bool InputManager::isKeyReleased(const Key key) const
{
	return !currentKeyStates.at(key) && previousKeyStates.at(key);
}

std::string InputManager::getHumanReadableKey(const Key key) const
{
	for (auto &[code, storedKey] : scancodeMapping) {
		if (storedKey == key) {
			const SDL_Keycode keycode = SDL_GetKeyFromScancode(code, SDL_KMOD_NONE, false);
			const char *keyName = SDL_GetKeyName(keycode);
			return keyName;
		}
	}
	return "Unknown";
}
