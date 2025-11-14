#include "InputManager.hpp"
#include "Engine/DebugSystem.hpp"

InputManager::InputManager()
{
	scancodeMapping = {{SDL_SCANCODE_A, Key::Left}, {SDL_SCANCODE_D, Key::Right}, {SDL_SCANCODE_SPACE, Key::Up},
	                   {SDL_SCANCODE_S, Key::Down}, {SDL_SCANCODE_E, Key::Interact}, {SDL_SCANCODE_K, Key::Attack}};

	for (auto &[code, key] : scancodeMapping) {
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

bool InputManager::isKeyDown(Key key) const
{
	return currentKeyStates.at(key);
}

bool InputManager::isKeyPressed(Key key) const
{
	return currentKeyStates.at(key) && !previousKeyStates.at(key);
}

bool InputManager::isKeyReleased(Key key) const
{
	return !currentKeyStates.at(key) && previousKeyStates.at(key);
}
