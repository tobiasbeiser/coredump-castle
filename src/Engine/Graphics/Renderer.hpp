#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <memory>
#include <vector>

struct SDLTextureDeleter {
	void operator()(SDL_Texture* t) const {
		if (t) SDL_DestroyTexture(t);
	}
};

class AnimatedGameObject;
class GameObject;

class Renderer {
public:
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(const std::string& title, int w, int h);
	~Renderer();

	void StartFrame(int red, int green, int blue, int alpha) const;
	void EndFrame() const;
	void DrawAnimatedGameObject(const AnimatedGameObject& obj);
	void DrawGameObject(const GameObject& gameObject, const float srcX, const float srcY, const float dstX, const float dstY);
	void DrawTexture(SDL_Texture& texture, const float srcX, const float srcY, const float dstX, const float dstY, const float spriteSize) const;
	void DrawBackground(SDL_Texture& texture) const;
	void DrawParalaxBackground(SDL_Texture& texture, float& scrollPos, float y, float scale) const;
	void CenterCameraOn(const GameObject& gameObject);


	void Resize(int w, int h);

	std::unique_ptr<SDL_Texture, SDLTextureDeleter> LoadTexture(const std::string& filePath);

private:

	int width;
	int height;
	const int logW = 768;
	const int logH = 432;

	SDL_FRect viewport{ .x = 0, .y = 0, .w = static_cast<float>(logW), .h = static_cast<float>(logH) };
	std::vector<std::string> debugMessages;
	void RenderTexture(SDL_Texture& texture, const SDL_FRect& srcRect, SDL_FRect& dstRect, bool flip) const;

	struct SDLRendererDeleter { void operator()(SDL_Renderer* r) const noexcept { if (r) SDL_DestroyRenderer(r); } };
	struct SDLWindowDeleter { void operator()(SDL_Window* w) const  noexcept { if (w) SDL_DestroyWindow(w); } };

	std::unique_ptr<SDL_Renderer, SDLRendererDeleter> renderer;
	std::unique_ptr<SDL_Window, SDLWindowDeleter> window;
};