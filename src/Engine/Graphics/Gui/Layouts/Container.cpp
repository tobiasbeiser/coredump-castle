
#include "Container.hpp"

#include <ranges>

namespace Ui {
void Container::addWidget(std::unique_ptr<Widget> widget)
{
	children.push_back(std::move(widget));
}

bool Container::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	const auto childMousePos = localMousePos - getRelativePosition();

	// Reverse iteration for event bubbling (top-most child first)
	for (const auto &child : std::ranges::reverse_view(getChildren())) {
		if (child->handleEvent(childMousePos, event))
			return true;
	}
	return false;
}

void Container::draw(const UIContext &context, const glm::vec2 parentOffset) const
{
	// const auto rect = getScreenRect(parentOffset);
	//
	// SDL_SetRenderDrawColor(context.renderer->getNativeRenderer(), 255, 0, 0, 0);
	// SDL_RenderRect(context.renderer->getNativeRenderer(), &rect);

	const auto selfOffset = parentOffset + getRelativePosition();
	for (const auto &child : getChildren()) {
		child->draw(context, selfOffset);
	}
}

const std::vector<std::unique_ptr<Widget>> &Container::getChildren() const
{
	return children;
}
} // namespace Ui
