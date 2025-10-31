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

		auto view = registry.view<InputComponent>();
		for (auto &entity : view) {
			auto &input = view.get<InputComponent>(entity);
			input.horizontalDirection = horizontalDirection;
			input.jumpPressed = inputManager.IsKeyPressed(Key::Up);
			input.meleePressed = inputManager.IsKeyPressed(Key::Melee);

			//SDL_Log("Before %d %d", input.interactPressed, input.interactConsumed);
			std::string before = std::to_string(static_cast<int>(input.interactPressed)) + " "
			                                    + std::to_string(static_cast<int>(input.interactConsumed));
			PressOrConsume(inputManager.IsKeyPressed(Key::Interact), input.interactPressed, input.interactConsumed);
			std::string after = std::to_string(static_cast<int>(input.interactPressed)) + " "
			                                    + std::to_string(static_cast<int>(input.interactConsumed));

			//SDL_Log("InputSystem: Entity %d Interact before: %s after: %s", entity, before.c_str(), after.c_str());
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