#pragma once

enum class AiState {
	Idle,
	Patrol,
	Chase,
	Attack,
};

struct AiStateComponent {
	AiState state = AiState::Idle;
};