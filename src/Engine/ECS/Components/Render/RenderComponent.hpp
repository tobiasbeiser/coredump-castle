#pragma once

struct RenderComponent {
	int layer = 0;
	bool flipX = false;
	bool flipXOffset = false;
	int alpha = 255;

	[[nodiscard]] bool isFacingLeft() const { return flipX ^ flipXOffset; }

	void setFacingLeft(const bool facingLeft) { flipX = facingLeft ^ flipXOffset; }
};