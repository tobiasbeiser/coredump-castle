#include "InputManager.hpp"
#include "DebugSystem.hpp"

InputManager::InputManager() {
	scancodeMapping = {
		{SDL_SCANCODE_A, Key::Left },
		{SDL_SCANCODE_D, Key::Right },
		{SDL_SCANCODE_W, Key::Up },
		{SDL_SCANCODE_S, Key::Down },
	};

	for (auto& [code, key] : scancodeMapping) {
		previousKeyStates[key] = false;
		currentKeyStates[key] = false;
	}
}

void InputManager::Update(const SDL_KeyboardEvent* e) {
	previousKeyStates = currentKeyStates;
	if (scancodeMapping.find(e->scancode) != scancodeMapping.end()) {
		const Key key = scancodeMapping.at(e->scancode);
		currentKeyStates[key] = e->down;
		if (key == Key::Up) {
			SDL_Log("Key %d is %s was %s", static_cast<int>(key), e->down ? "down" : "up", previousKeyStates[key] ? "down" : "up");

		}
	}
}


bool InputManager::IsKeyDown(Key key) const {
	return currentKeyStates.at(key);
}

bool InputManager::IsKeyPressed(Key key) const {
	return currentKeyStates.at(key) && !previousKeyStates.at(key);
}

bool InputManager::IsKeyReleased(Key key) const {
	return !currentKeyStates.at(key) && previousKeyStates.at(key);
}
