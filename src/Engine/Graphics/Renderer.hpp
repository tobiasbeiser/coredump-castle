#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct SDLTextureDeleter {
	void operator()(SDL_Texture *t) const
	{
		if (t)
			SDL_DestroyTexture(t);
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

	void startFrame(int red, int green, int blue, int alpha) const;
	void endFrame() const;
	void centerCameraOn(const glm::vec2 &position, const glm::vec2 &levelPos, const glm::vec2 &levelSize);

	void resize(Resolution newResolution);

	std::unique_ptr<SDL_Texture, SDLTextureDeleter> loadTexture(const std::filesystem::path &filePath) const;
	void renderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> srcRect, std::optional<SDL_FRect> dstRect,
	                   const bool flip, double angle = 0.0) const;
	void renderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scrollFactor) const;

	void renderCircularFade(float progress, SDL_FColor color = {0, 0, 0, 255});

  private:
	void setupImGui() const;
	static void startImGuiFrame();
	Resolution currentResolution{1920, 1080};
	Resolution logicalResolution{768, 432};

	SDL_FRect viewport{.x = 0,
	                   .y = 0,
	                   .w = static_cast<float>(logicalResolution.width),
	                   .h = static_cast<float>(logicalResolution.height)};
	std::vector<std::string> debugMessages{};

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

	std::unique_ptr<SDL_Renderer, SDLRendererDeleter> renderer{};
	std::unique_ptr<SDL_Window, SDLWindowDeleter> window{};
};