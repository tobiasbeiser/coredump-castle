#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <filesystem>
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

struct SDLTextureDeleter {
	void operator()(SDL_Texture *t) const
	{
		if (t)
			SDL_DestroyTexture(t);
	}
};

struct TTFFontCloser {
	void operator()(TTF_Font *f) const
	{
		if (f)
			TTF_CloseFont(f);
	}
};

struct TTFTextDestroyer {
	void operator()(TTF_Text *t) const
	{
		if (t)
			TTF_DestroyText(t);
	}
};

class AnimatedGameObject;
class GameObject;

struct Resolution {
	int width;
	int height;
};

class Renderer {
  public:
	Renderer(const std::string &title, const Resolution resolution);
	// delete copy, move and assignments as we want only one instance of the renderer (create one at start that lives
	// until shutdown) Cannot be copied as it manages a unique resource (the SDL_Renderer and SDL_Window)
	Renderer(const Renderer &) = delete;
	Renderer(Renderer &&) = delete;
	Renderer &operator=(Renderer &&) = delete;
	Renderer &operator=(const Renderer &) = delete;
	~Renderer();

	struct RenderScopeState {
		SDL_FRect logicalViewport{};
		std::optional<SDL_FRect> physicalClip;
		float scaleX = 0;
		float scaleY = 0;
	};

	class RenderScopeGuard {
	  public:
		RenderScopeGuard(Renderer &r, const SDL_FRect &screenRect, float zoom);
		~RenderScopeGuard();
		RenderScopeGuard(const RenderScopeGuard &) = delete;
		RenderScopeGuard(const RenderScopeGuard &&) = delete;
		RenderScopeGuard &operator=(const RenderScopeGuard &) = delete;
		RenderScopeGuard &operator=(const RenderScopeGuard &&) = delete;

	  private:
		Renderer &renderer;
	};

	void pushRenderScope(const SDL_FRect &screenRect, float zoom);
	void popRenderScope();
	[[nodiscard]] RenderScopeGuard createRenderScope(const SDL_FRect &screenRect, const float zoom);

	void startFrame(int red, int green, int blue, int alpha) const;
	void endFrame() const;
	void setCameraCenter(const glm::vec2 &newCenter);
	void centerCameraOn(const glm::vec2 &position, const glm::vec2 &levelPos, const glm::vec2 &levelSize);

	void resize(Resolution newResolution);
	[[nodiscard]] Resolution getLogicalResolution() const;
	[[nodiscard]] glm::vec2 windowToLogical(float windowX, float windowY) const;

	[[nodiscard]] SDL_Renderer *getNativeRenderer() const;
	[[nodiscard]] TTF_TextEngine *getTextEngine() const;
	void renderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> &srcRect,
	                   const std::optional<SDL_FRect> &dstRect, const bool flip, double angle = 0.0) const;
	void renderAbsolutePositionTexture(SDL_Texture &texture, const std::optional<SDL_FRect> &srcRect,
	                                   const std::optional<SDL_FRect> &dstRect, bool flip, double angle) const;

	[[nodiscard]] std::unique_ptr<SDL_Texture, SDLTextureDeleter>
	loadTexture(const std::filesystem::path &filePath) const;
	static std::unique_ptr<TTF_Font, TTFFontCloser> loadFont(const std::filesystem::path &filePath, int fontSize);
	std::unique_ptr<TTF_Text, TTFTextDestroyer> createText(TTF_Font *font, const std::string &text) const;
	void drawRectShape(SDL_FRect rect, SDL_Color fillColor, SDL_Color outlineColor) const;
	void drawText(TTF_Text *textObject, float x, float y) const;
	void renderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, float scrollFactor) const;
	void renderCircularFade(float progress, SDL_FColor color = {0, 0, 0, 255});
	void renderVignette(float intensity, const SDL_FColor &color,
	                    const std::optional<glm::vec2> &focalPoint = std::nullopt) const;
	[[nodiscard]] SDL_FRect getViewport() const;

  private:
	void setupImGui() const;
	static void startImGuiFrame();
	Resolution currentResolution{1920, 1080};
	Resolution logicalResolution{768, 432};

	SDL_FRect viewport{.x = 0,
	                   .y = 0,
	                   .w = static_cast<float>(logicalResolution.width),
	                   .h = static_cast<float>(logicalResolution.height)};
	SDL_FRect mapViewport{.x = 0,
	                      .y = 0,
	                      .w = static_cast<float>(logicalResolution.width),
	                      .h = static_cast<float>(logicalResolution.height)};

	std::vector<std::string> debugMessages{};

	std::stack<RenderScopeState> stateStack;

	struct SDLRendererDeleter {
		void operator()(SDL_Renderer *r) const noexcept
		{
			if (r)
				SDL_DestroyRenderer(r);
		}
	};
	struct SDLWindowDeleter {
		void operator()(SDL_Window *w) const noexcept
		{
			if (w)
				SDL_DestroyWindow(w);
		}
	};
	struct TTFTextEngineDeleter {
		void operator()(TTF_TextEngine *e) const noexcept
		{
			if (e)
				TTF_DestroyRendererTextEngine(e);
		}
	};

	std::unique_ptr<TTF_TextEngine, TTFTextEngineDeleter> textEngine{};
	std::unique_ptr<SDL_Renderer, SDLRendererDeleter> renderer{};
	std::unique_ptr<SDL_Window, SDLWindowDeleter> window{};
};