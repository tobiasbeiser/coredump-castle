#pragma once

struct AttackStateComponent {
    bool isAttacking = false;
    float attackTimer = 0.0f;
    float attackCooldownTimer = 0.0f;
};
