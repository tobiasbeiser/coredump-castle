#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Input/InputManager.hpp"
#include <iostream>

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
			// add logging here
			if (input.meleePressed) {
				std::cout << "Entity " << entity << " pressed melee" << std::endl;
			}
		}
	}

  private:
	const InputManager &inputManager;
};
} // namespace GameEngine::ECS::Systems