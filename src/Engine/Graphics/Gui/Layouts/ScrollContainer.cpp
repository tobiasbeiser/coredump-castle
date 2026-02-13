#include "ScrollContainer.hpp"

#include "Engine/Utils/RenderUtils.hpp"

#include <algorithm>
#include <ranges>

namespace Ui {

bool ScrollContainer::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	const SDL_FRect bounds = getBounds();
	const SDL_FPoint mousePt = {localMousePos.x, localMousePos.y};
	const bool isInside = SDL_PointInRectFloat(&mousePt, &bounds);

	if (event.type == SDL_EVENT_MOUSE_WHEEL && isInside) {
		float maxChildY = 0.0f;
		for (const auto &child : children) {
			maxChildY = std::max(maxChildY, child->getRelativePosition().y + child->getSize().y);
		}

		const float maxScroll = std::max(0.0f, maxChildY - bounds.h);

		scrollY -= event.wheel.y * 10.0f;
		scrollY = std::clamp(scrollY, 0.0f, maxScroll);
		return true;
	}

	if (!isInside && event.type != SDL_EVENT_MOUSE_MOTION) {
		return false;
	}

	const auto childMousePos =
	    isInside ? (localMousePos - getRelativePosition() + glm::vec2{0.0f, scrollY}) : glm::vec2{-99999.0f, -99999.0f};

	for (const auto &child : std::ranges::reverse_view(children)) {
		if (child->handleEvent(childMousePos, event))
			return true;
	}
	return false;
}

void ScrollContainer::draw(const UIContext &context, const glm::vec2 parentOffset) const
{
	const auto fRect = getScreenRect(parentOffset);
	const SDL_Rect clipRect = GameEngine::Utils::RenderUtils::rectFromFRect(fRect);
	GameEngine::Utils::RenderUtils::withClipRect(context.renderer->getNativeRenderer(), clipRect, [&] {
		const auto selfOffset = parentOffset + getRelativePosition() - glm::vec2{0.0f, scrollY};
		for (const auto &child : children) {
			child->draw(context, selfOffset);
		}
	});
}

} // namespace Ui