#pragma once

struct InputComponent {
	float horizontalDirection{0.0f};
	bool jumpPressed{false};
	bool meleePressed{false};
	bool meleeConsumed{false};
	bool interactPressed{false};
	bool interactConsumed{false};
};
