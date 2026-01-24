#pragma once
#include "Container.hpp"
#include <glm/vec2.hpp>
#include <vector>

namespace Ui {
// Defines how a child is positioned within an AnchorLayout
struct AnchorParams {
	// Position on the Parent (0.0 to 1.0).
	// (0,0) = TopLeft, (0.5, 0.5) = Center, (1,1) = BottomRight
	glm::vec2 anchor = {0.0f, 0.0f};

	// Point on the Child to align with the Anchor (0.0 to 1.0).
	// (0.5, 0.5) ensures the child's center is exactly at the anchor point.
	glm::vec2 pivot = {0.0f, 0.0f};

	// Pixel offset for fine-tuning
	glm::vec2 offset = {0.0f, 0.0f};

	// Common Presets
	static const AnchorParams center;
	static const AnchorParams topLeft;
	static const AnchorParams topRight;
	static const AnchorParams bottomCenter;
};

class AnchorLayout : public Container {
  public:
	AnchorLayout() { setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent); }

	void addChild(std::unique_ptr<Widget> widget, AnchorParams params);

	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;

  protected:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;

  private:
	// Parallel vector to base class 'children' to store layout data.
	std::vector<AnchorParams> anchorParams;
};
} // namespace Ui
