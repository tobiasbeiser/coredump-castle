#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
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
	Renderer(const std::string &title, Resolution resolution);
	// delete copy, move and assignments as we want only one instance of the renderer (create one at start that lives
	// until shutdown) Cannot be copied as it manages a unique resource (the SDL_Renderer and SDL_Window)
	Renderer(const Renderer &) = delete;
	Renderer(Renderer &&) = delete;
	Renderer &operator=(Renderer &&) = delete;
	Renderer &operator=(const Renderer &) = delete;
	~Renderer();

	void StartFrame(int red, int green, int blue, int alpha) const;
	void EndFrame() const;
	void CenterCameraOn(const float positionX);

	void Resize(Resolution newResolution);

	std::unique_ptr<SDL_Texture, SDLTextureDeleter> LoadTexture(const std::filesystem::path &filePath);
	void RenderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> srcRect, std::optional<SDL_FRect> dstRect,
	                   const bool flip) const;
	void RenderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scrollFactor) const;

	void RenderCircularFade(float progress, SDL_FColor color = {0, 0, 0, 255});

  private:
	Resolution currentResolution{1920, 1080};
	Resolution logicalResolution{768, 432};
	const int logW = 768;
	const int logH = 432;

	SDL_FRect viewport{.x = 0,
	                   .y = 0,
	                   .w = static_cast<float>(logicalResolution.width),
	                   .h = static_cast<float>(logicalResolution.height)};
	std::vector<std::string> debugMessages;

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

	std::unique_ptr<SDL_Renderer, SDLRendererDeleter> renderer;
	std::unique_ptr<SDL_Window, SDLWindowDeleter> window;
};