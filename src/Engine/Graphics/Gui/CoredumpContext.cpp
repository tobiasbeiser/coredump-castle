

#include "Core/UiContext.hpp"
#include "Engine/GameEngine.hpp"

namespace CoredumpContext
{
	using namespace Ui;

	std::unique_ptr<Ui::UIContext> getContext(GameEngine::GameEngine& game)
	{
		ButtonStyle btnStyle = {
			10.0f,
			5.0f,
			// Normal: Dark Grey Fill, White Outline, WHITE Text
			Paint{{30, 30, 30, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}},
			// Hovered: Blue Fill, Yellow Outline, WHITE Text
			Paint{{50, 50, 150, 255}, {255, 255, 0, 255}, {255, 255, 255, 255}},
			// Pressed: Lighter Grey Fill, Grey Outline, WHITE Text
			Paint{{100, 100, 100, 255}, {200, 200, 200, 255}, {255, 255, 255, 255}},
			Paint{{150, 150, 150, 255}, {200, 200, 200, 255}, {215, 215, 215, 215}},
		};
		UiTheme theme = {{255, 215, 0, 255}, btnStyle};

		return std::make_unique<UIContext>(&game.getRenderer(),
		                                   game.getFontManager().getFont("assets/fonts/upheaval/upheavtt.ttf", 60),
		                                   // Upheaval font is designed for size 20, use only multiples for crips fonts
		                                   game.getFontManager().getFont("assets/fonts/upheaval/upheavtt.ttf", 20),
		                                   // aansa font is designed for size 16, use only multiples for crips fonts
		                                   game.getFontManager().getFont("assets/fonts/aansa/aansa.ttf", 16), theme,
		                                   game.getRenderer().getLogicalResolution());
	}
} // namespace CoredumpContext
