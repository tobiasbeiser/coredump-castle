#pragma once

struct InputComponent {
	float horizontalDirection{0.0f};
	bool jumpPressed{false};
	bool attackPressed{false};
	bool attackConsumed{false};
	bool interactPressed{false};
	bool interactConsumed{false};
};
