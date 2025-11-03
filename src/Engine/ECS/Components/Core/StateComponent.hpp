#pragma once

enum States { Idle, Walking, Jumping, Falling, Attacking, Crouching, ConsecutiveAttacking };

struct StateComponent {
	int state = 0;
};