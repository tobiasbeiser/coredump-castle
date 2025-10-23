#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

struct SDLTextureDeleter {
	void operator()(SDL_Texture *t) const
	{
		if (t)
			SDL_DestroyTexture(t);
	}
};

class AnimatedGameObject;
class GameObject;

class Renderer {
  public:
	// delete copy, move and assignments as we want only one instance of the renderer (create one at start that lives
	// until shutdown) Cannot be copied as it manages a unique resource (the SDL_Renderer and SDL_Window)
	Renderer(const Renderer &) = delete;
	Renderer(Renderer &&) = delete;
	Renderer &operator=(Renderer &&) = delete;
	Renderer &operator=(const Renderer &) = delete;
	Renderer(const std::string &title, int w, int h);
	~Renderer();

	void StartFrame(int red, int green, int blue, int alpha) const;
	void EndFrame() const;
	void RenderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scale, const float scrollFactor) const;
	void CenterCameraOn(const int positionX);
	SDL_Renderer& getSDLRenderer() const { return *renderer; }

	void Resize(int w, int h);

	std::unique_ptr<SDL_Texture, SDLTextureDeleter> LoadTexture(const std::filesystem::path &filePath);
	void RenderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> srcRect, std::optional<SDL_FRect> dstRect,
	                   const bool flip) const;

  private:
	int width;
	int height;
	const int logW = 768;
	const int logH = 432;

	SDL_FRect viewport{.x = 0, .y = 0, .w = static_cast<float>(logW), .h = static_cast<float>(logH)};
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