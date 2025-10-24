#include "Renderer.hpp"
#include "Engine/DebugSystem.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdexcept>
#include <numbers>

Renderer::Renderer(const std::string &title, Resolution resolution) : currentResolution(resolution)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		throw std::runtime_error(SDL_GetError());
	}

	window.reset(
	    SDL_CreateWindow(title.c_str(), currentResolution.width, currentResolution.height, SDL_WINDOW_RESIZABLE));
	if (!window)
		throw std::runtime_error(SDL_GetError());

	renderer.reset(SDL_CreateRenderer(window.get(), nullptr));
	if (!renderer)
		throw std::runtime_error(SDL_GetError());

	SDL_SetRenderLogicalPresentation(renderer.get(), logW, logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_SetRenderVSync(renderer.get(), 1);
}

Renderer::~Renderer()
{
	SDL_Quit();
}

void Renderer::Resize(Resolution newResolution)
{
	currentResolution = newResolution;
	SDL_SetWindowSize(window.get(), currentResolution.width, currentResolution.height);
}

std::unique_ptr<SDL_Texture, SDLTextureDeleter> Renderer::LoadTexture(const std::filesystem::path &filePath)
{
	SDL_Texture *tex = IMG_LoadTexture(renderer.get(), filePath.string().c_str());
	if (!tex) {
		throw std::runtime_error(SDL_GetError());
	}
	SDL_SetTextureScaleMode(tex, SDL_ScaleMode::SDL_SCALEMODE_NEAREST);

	return std::unique_ptr<SDL_Texture, SDLTextureDeleter>(tex);
}

void Renderer::StartFrame(int red, int green, int blue, int alpha) const
{
	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);
	SDL_RenderClear(renderer.get());
}

void Renderer::EndFrame() const
{
	for (auto &debug : DebugSystem::Instance().debugRects) {
		SDL_SetRenderDrawColor(renderer.get(), debug.color.r, debug.color.g, debug.color.b, debug.color.a);
		SDL_RenderRect(renderer.get(), &debug.rect);
	}

	std::string DebugMessage;
	for (std::string message : DebugSystem::Instance().debugMessages) {
		DebugMessage.append(message + "\n");
	}
	const std::uint8_t red = 255, green = 255, blue = 255, alpha = 255;
	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);

	const float debugTextX = 5, debugTextY = 5;
	SDL_RenderDebugText(renderer.get(), debugTextX, debugTextY, DebugMessage.c_str());

	SDL_RenderPresent(renderer.get());
}

void Renderer::CenterCameraOn(const float positionX)
{
	const float newCenter = positionX - (viewport.w / 2);
	const int tileSize = 32;
	viewport.x = newCenter > tileSize ? newCenter : tileSize;
}

void Renderer::RenderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scrollFactor) const
{
	const float repetitions = 5.0f;
	dstRect.x = -viewport.x * scrollFactor;
	dstRect.w = static_cast<float>(texture.w) * repetitions;

	SDL_RenderTextureTiled(renderer.get(), &texture, nullptr, 1, &dstRect);
}

void Renderer::RenderCircularFade(float progress, SDL_FColor color)
{
	if (progress <= 0.0f)
		return;

	if (progress > 1.0f)
		progress = 1.0f;

	const auto [width, height] = std::make_pair(logicalResolution.width, logicalResolution.height);
	float maxRadius = std::sqrt(width * width + height * height);
	float radius = maxRadius * progress;

	const int segments = 64;
	std::vector<SDL_Vertex> vertices(segments + 2);

	vertices[0].position = {width / 2.0f, height / 2.0f};
	vertices[0].color = color;

for (int i = 0; i <= segments; ++i) {
		float angle = 2.0f * std::numbers::pi * i / segments;
		float x = width / 2.0f + radius * cos(angle);
		float y = height / 2.0f + radius * sin(angle);
		vertices[i + 1].position = {x, y};
		vertices[i + 1].color = color;
	}

	std::vector<int> indices;
	for (int i = 1; i <= segments; ++i) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i + 1);
	}

	SDL_RenderGeometry(renderer.get(), nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}

void Renderer::RenderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> srcRect,
                             std::optional<SDL_FRect> dstRect, const bool flip) const
{
	const SDL_FRect *srcPtr = srcRect ? &srcRect.value() : nullptr;
	SDL_FRect *dstPtr = nullptr;

	if (dstRect) {
		dstRect.value().x -= viewport.x;
		dstPtr = &dstRect.value();
	}
	const SDL_FlipMode flipMode = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	SDL_RenderTextureRotated(renderer.get(), &texture, srcPtr, dstPtr, 0, nullptr, flipMode);
}
