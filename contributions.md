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
            - RangedActions
        - StateMachine
            - EnemyStateMachineFactory
            - StateMachineBuilder
    - Factories
        - ProjectileFactory
        - EnemyFactory
- Tests (related system tests from topics above)

## Leo
