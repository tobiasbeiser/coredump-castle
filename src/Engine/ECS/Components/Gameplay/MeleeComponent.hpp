struct MeleeComponent {
	bool isAttacking{false};
	float range{10.0f};
	int damage{10}; // depends on health system. For now it will not be used
	float attackDuration{0.6f};
	float attackCooldown{1.0f};
	float attackTimer{0.0f};
	float hitboxActiveStart{0.15f}; // needs to be changed based on animation
	float hitboxActiveEnd{0.35f};
	float comboInputWindow{0.5f};
	bool comboInputReceived{false};
};