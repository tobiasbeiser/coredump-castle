# Contributions

## Tobias

- GameEngine - `src/Engine/GameEngine.[hpp/cpp]`
- ECS
    - Registry
    - Storage - `src/Engine/ECS/ComponentStorage.hpp`
    - View - `src/Engine/ECS/View.hpp`
- PlayState + fixed Timesteps
- TextureManager
- Audio Manager
- InputManager
- Renderer
- ImGui/Profiler
- Tracy + Performance optimizations

- Systems
    - AnimationSystem
    - BlinkSystem
    - CollisionSystem
    - DeathSystem
    - EffectSystem
    - InputSystem
    - JumpSystem
    - MapActivationSystem
    - MovementSystem
    - PhysicsSystem
    - RenderSystem
    - RespawnSystem
    - SoundSystem
    - StateSystem
    - TooltipSystem
    - VignetteSystem
- Game
    - Tiled maps
    - Tilesets
    - WorldFactory
        - LevelFactory
        - TileFactory
        - AnimationFactory
        - EffectFactory
        - PlayerFactory
- CMake/vcpkg
- CI/CD
    - Pipelines
    - Rules

## Erik

Main Responsibility for the whole Enemies + Boss, Combat system and Enemy related AI.

- Setup
    - Google Tests
- Debug
    - DebugSystem (in collaboration)
- System
    - Combat
        - AttackSystem
        - HealthSystem
        - HitDetectionSystem
        - PoiseSystem
    - Enemy
        - ChargeSystem
        - ChaseSystem
        - PatrolSystem
        - ProjectileSystem
        - BlinkSystem
        - CameraShakeSystem
        - ChargeTelegraphSystem
        - StateMachineSystem
    - Utils
        - SpatialHash
- Game
    - AI
        - Actions
            - CoreActions
            - MeleeActions
            - RangedActions `src/Game/AI/Actions/RangedActions.[hpp/cpp]`
        - StateMachine
            - EnemyStateMachineFactory `src/Game/AI/StateMachine/EnemyStateMachineFactory.hpp`
            - StateMachineBuilder `src/Game/AI/StateMachine/StateMachineBuilder.cpp`
    - Factories
        - ProjectileFactory
        - EnemyFactory
- Tests (related system tests from topics above)

## Leo

Mainly responsible for the UI, Menu States and saving.

- Engine
  - Systems
    - ProgressSystem
    - HudSystem
    - UiRenderSystem
    - JumpSystem (for double jump)
    - MapRenderSystem
    - MapTrackingSystem
    - MenuStateToggleSystem
    - RenderSystem (refactoring and extension for HUD)
    - ReviveSystem
    - TooltipSystem
  - FileIO
    - FileSaveStorage
    - ISaveStorage
    - SaveManager
  - Graphics
    - Gui
      - Widget
      - UiContext
      - UiTypes
      - Container
      - ScrollContainer
      - Button
      - Frame
      - Image
      - Label
      - CoredumpContext
      - LayoutEngine `src/Engine/Graphics/Gui/LayoutEngine.[hpp/cpp]`
      - UiBuilder `src/Engine/Graphics/Gui/UiBuilder.hpp`
      - UiViewFactory `src/Engine/Graphics/Gui/UiViewFactory.[hpp/cpp]`
      - Utils
    - Renderer (Ui-related parts)
  - Utils
    - ReferenceFrameUtils
    - RenderUtils
    - SaveGameFormatter
    - TimeUtils
- Game
  - Factories
    - OverlayFactory
  - States
    - GameFinishedState
    - GameLoadingState
    - GameOverState
    - MainMenuState
    - MapMenuState
    - PauseMenuState
- Tests relating to saving, timeutils, jumpSystem