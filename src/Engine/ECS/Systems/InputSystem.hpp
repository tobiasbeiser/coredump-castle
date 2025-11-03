#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Input/InputManager.hpp"
#include <string>

namespace GameEngine::ECS::Systems {
class InputSystem {
  public:
	InputSystem(const InputManager &inputManager) : inputManager(inputManager) {}

	void Update(Registry &registry)
	{

		float horizontalDirection = 0;
		if (inputManager.IsKeyDown(Key::Left)) {
			horizontalDirection += -1;
		}
		if (inputManager.IsKeyDown(Key::Right)) {
			horizontalDirection += 1;
		}	

		for (auto view = registry.view<InputComponent>(); const auto &entity : view) {
			auto &input = view.get<InputComponent>(entity);
			input.horizontalDirection = horizontalDirection;
			PressOrConsume(inputManager.IsKeyPressed(Key::Up), input.jumpPressed, input.jumpConsumed);
			PressOrConsume(inputManager.IsKeyPressed(Key::Interact), input.interactPressed, input.interactConsumed);
		}
	}

	// set an input and consume it in the next frame
	// if the input was already consumed, do not set it again (We update keyboard events once per frame, however we can
	// have multiple updates per frame)

  private:
	const InputManager &inputManager;

	void PressOrConsume(bool keyPressed, bool &pressedProp, bool &consumedProp)
	{
		if (keyPressed) {
			bool inPressedProp = pressedProp;
			bool inConsumedProp = consumedProp;
			if (!consumedProp) {
				pressedProp = true;
				consumedProp = true;
			} else {
				pressedProp = false;
			}
			//SDL_Log(
			//    "PressOrConsume: keyPressed=%d, inPressedProp=%d, inConsumedProp=%d => pressedProp=%d, consumedProp=%d",
			//    keyPressed, inPressedProp, inConsumedProp, pressedProp, consumedProp);
		} else {
			pressedProp = false;
			consumedProp = false;
		}
	}
};
} // namespace GameEngine::ECS::Systems