#pragma once


struct JumpComponent {
	float jumpStrength = 380;
	bool allowDoubleJump = true;
	int jumpsLeft = 1;
	int maxJumps = 2;
};