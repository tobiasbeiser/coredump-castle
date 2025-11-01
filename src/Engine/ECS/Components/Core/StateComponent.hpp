#pragma once

enum States { Idle, Walking, Jumping, Falling, Attacking, Crouching, ConsecutiveAttack };

struct StateComponent {
	int state = 0;
};