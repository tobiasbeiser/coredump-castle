#pragma once
struct ComboComponent {
    bool comboInputReceived = false; 
    bool isComboAttack = false;
    float comboInputWindow{};
    float comboAttackDuration{};
};