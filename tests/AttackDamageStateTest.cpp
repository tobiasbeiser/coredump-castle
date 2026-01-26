#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

struct AttackDamageStateTest : testing::Test {
    GameEngine::ECS::Registry registry;
    GameEngine::ECS::Entity entity;

    void SetUp() override {
        entity = registry.create();
        registry.registerComponent<AttackComponent>(entity, {
            .attackTimer = Timer{1.0s},
            .attackCoolDownTimer = Timer{0.0s},
            .windUpLimit = 1.0f,
            .activeWindUpLimit = 0.5f // Damage active after 50%
        });
        registry.registerComponent<InputComponent>(entity, InputComponent{});
    }

    AttackComponent& getAttack() {
        return registry.view<AttackComponent>().get<AttackComponent>(entity);
    }
    
    InputComponent& getInput() {
        return registry.view<InputComponent>().get<InputComponent>(entity);
    }
};

TEST_F(AttackDamageStateTest, SetsDamageActiveBasedOnTimerWithoutAnimation) {
    auto& input = getInput();
    auto& attack = getAttack();
    
    input.attackPressed = true;
    GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f, [](auto){});
    
    EXPECT_FALSE(attack.isDamageActive);
    
    GameEngine::ECS::Systems::AttackSystem::update(registry, 0.5f, [](auto){});
    EXPECT_TRUE(attack.isDamageActive);
}

TEST_F(AttackDamageStateTest, SetsDamageActiveAlwaysTrueIfNoWindUp) {
    auto& input = getInput();
    auto& attack = getAttack();
    attack.activeWindUpLimit = 0.0f;
    
    input.attackPressed = true;
    GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f, [](auto){});
    
    EXPECT_TRUE(attack.isDamageActive);
}
