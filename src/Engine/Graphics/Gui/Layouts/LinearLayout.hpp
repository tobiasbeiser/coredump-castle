#pragma once
#include "Container.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {
class LinearLayout : public Container {
  public:
	explicit LinearLayout(Orientation orient = Orientation::Vertical, float gap = 5.0f);

	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	void setOrientation(Orientation newOrientation);
	void setCrossAxisAlignment(CrossAxisAlignment align);
	void setSpacing(float gap);

  private:
	[[nodiscard]] glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;
	[[nodiscard]] InternalDimensions measureFixedChildren(bool isVert, BoxConstraints constraints,
	                                                      const UIContext &ctx) const;
	void measureFlexibleChildren(InternalDimensions &tmpDimensions, bool isVert, BoxConstraints constraints,
	                             const UIContext &ctx) const;

	Orientation orientation;
	float spacing;
	CrossAxisAlignment crossAxisAlignment = CrossAxisAlignment::Center;
};
} // namespace Ui
