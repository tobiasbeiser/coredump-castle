#pragma once
#include <memory>
#include <SDL3/SDL_events.h>

#include "Base/Widget.hpp"
#include "Core/UiContext.hpp"


namespace Ui
{

	class GuiManager {
	public:
		void setRoot(std::unique_ptr<Widget> root);

		void updateLayout(const UIContext &context, float screenWidth, float screenHeight) const;

		void handleEvent(const UIContext &ctx, const SDL_Event &event) const;
		void render(const UIContext &ctx) const;

	private:
		std::unique_ptr<Widget> rootWidget;
	};
}
