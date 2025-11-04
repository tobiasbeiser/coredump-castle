#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Input/InputManager.hpp"
#include <string>

namespace GameEngine::ECS::Systems {
class InputSystem {
  public:
	explicit InputSystem(InputManager &inputManager) : inputManager(inputManager) {}
	InputSystem(const InputSystem &) = delete;
	InputSystem(InputSystem &&) = delete;
	InputSystem &operator=(InputSystem &&) = delete;
	InputSystem &operator=(const InputSystem &) = delete;
	~InputSystem() = default;

	void update(Registry &registry)
	{

		float horizontalDirection = 0;
		if (inputManager.isKeyDown(Key::Left)) {
			horizontalDirection += -1;
		}
		if (inputManager.isKeyDown(Key::Right)) {
			horizontalDirection += 1;
		}

		auto view = registry.view<InputComponent>();
		for (auto &entity : view) {
			auto &input = view.get<InputComponent>(entity);
			input.horizontalDirection = horizontalDirection;
			input.jumpPressed = inputManager.isKeyPressed(Key::Up);
			input.meleePressed = inputManager.isKeyPressed(Key::Melee);
			input.interactPressed = inputManager.isKeyPressed(Key::Interact);
		}

		// update input manager for next frame (for correct handling of isKeyPressed and isKeyReleased, this has to be
		// at the end)
		inputManager.update();
	}

	// set an input and consume it in the next frame
	// if the input was already consumed, do not set it again (We update keyboard events once per frame, however we can
	// have multiple updates per frame)

  private:
	InputManager &inputManager;

	// TODO delete after attack and double jump have been merged
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
			// SDL_Log(
			//     "PressOrConsume: keyPressed=%d, inPressedProp=%d, inConsumedProp=%d => pressedProp=%d,
			//     consumedProp=%d", keyPressed, inPressedProp, inConsumedProp, pressedProp, consumedProp);
		} else {
			pressedProp = false;
			consumedProp = false;
		}
	}
};
} // namespace GameEngine::ECS::Systems