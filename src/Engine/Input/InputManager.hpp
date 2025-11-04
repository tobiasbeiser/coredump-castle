#pragma once
#include "Key.hpp"
#include <SDL3/SDL.h>
#include <unordered_map>

class InputManager {
  public:
	InputManager();
	void processEvent(const SDL_KeyboardEvent *event);
	void update();
	bool isKeyDown(Key key) const;
	bool isKeyPressed(Key key) const;  // Key was just pressed this frame
	bool isKeyReleased(Key key) const; // Key was just released this frame

  private:
	std::unordered_map<Key, bool> currentKeyStates;
	std::unordered_map<Key, bool> previousKeyStates;
	std::unordered_map<SDL_Scancode, Key> scancodeMapping;
};