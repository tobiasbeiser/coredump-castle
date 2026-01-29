#pragma once
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MapToggleComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
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

	void update(Registry &registry) const
	{
		PROFILE_SCOPE("InputSystem");

		float horizontalDirection = 0;
		if (inputManager.isKeyDown(Key::Left)) {
			horizontalDirection += -1;
		}
		if (inputManager.isKeyDown(Key::Right)) {
			horizontalDirection += 1;
		}

		for (auto view =
		         registry.view<InputComponent, MapToggleComponent>().exclude<EnemyTag, PendingRespawnComponent>();
		     const auto entity : view) {
			auto &[horizontalDirectionTarget, jumpPressed, attackPressed, interactPressed, interactConsumed] =
			    view.get<InputComponent>(entity);
			horizontalDirectionTarget = horizontalDirection;
			jumpPressed = inputManager.isKeyPressed(Key::Up);
			attackPressed = inputManager.isKeyPressed(Key::Attack);
			interactPressed = inputManager.isKeyPressed(Key::Interact);

			auto &[mapToggled, _] = view.get<MapToggleComponent>(entity);
			mapToggled = inputManager.isKeyPressed(Key::ToggleMap);
		}

		for (auto view = registry.view<InputComponent, PendingRespawnComponent>(); const auto entity : view) {
			auto &[horizontalDirectionTarget, jumpPressed, attackPressed, interactPressed, interactConsumed] =
			    view.get<InputComponent>(entity);
			horizontalDirectionTarget = 0.0f;
			jumpPressed = false;
			attackPressed = false;
			interactPressed = false;
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
};
} // namespace GameEngine::ECS::Systems