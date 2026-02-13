
#include "Container.hpp"

#include <ranges>

namespace Ui {

bool Container::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	const auto childMousePos = localMousePos - getRelativePosition();

	for (const auto &child : std::ranges::reverse_view(children)) {
		if (child->handleEvent(childMousePos, event))
			return true;
	}
	return false;
}

void Container::draw(const UIContext &context, const glm::vec2 parentOffset) const
{
	const auto rect = getScreenRect(parentOffset);

	SDL_SetRenderDrawColor(context.renderer->getNativeRenderer(), 255, 0, 0, 0);
	SDL_RenderRect(context.renderer->getNativeRenderer(), &rect);

	const auto selfOffset = parentOffset + getRelativePosition();
	for (const auto &child : children) {
		child->draw(context, selfOffset);
	}
}

void Container::adoptChildren(std::vector<std::unique_ptr<Widget>> newChildren)
{
	children = std::move(newChildren);
}

} // namespace Ui
