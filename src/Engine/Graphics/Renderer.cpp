#include "Renderer.hpp"
#include "Engine/DebugSystem.hpp"
#include "math.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

Renderer::Renderer(const std::string &title, const Resolution resolution) : currentResolution(resolution)
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

	SDL_SetRenderLogicalPresentation(renderer.get(), logicalResolution.width, logicalResolution.height,
	                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_SetRenderVSync(renderer.get(), 1);

	if (!TTF_Init()) {
		throw std::runtime_error(SDL_GetError());
	}

	textEngine.reset(TTF_CreateRendererTextEngine(renderer.get()));
}

Renderer::~Renderer()
{
	TTF_DestroyRendererTextEngine(textEngine.get());
	SDL_Quit();
	TTF_Quit();
}

void Renderer::resize(const Resolution newResolution)
{
	currentResolution = newResolution;
}

std::unique_ptr<SDL_Texture, SDLTextureDeleter> Renderer::loadTexture(const std::filesystem::path &filePath) const
{
	SDL_Texture *tex = IMG_LoadTexture(renderer.get(), filePath.string().c_str());
	if (!tex) {
		throw std::runtime_error(SDL_GetError());
	}
	SDL_SetTextureScaleMode(tex, SDL_ScaleMode::SDL_SCALEMODE_NEAREST);

	return std::unique_ptr<SDL_Texture, SDLTextureDeleter>(tex);
}

std::unique_ptr<TTF_Font, TTFFontCloser> Renderer::loadFont(const std::filesystem::path &filePath, const int fontSize)
{
	TTF_Font *font = TTF_OpenFont(filePath.string().c_str(), fontSize);
	if (!font) {
		throw std::runtime_error(SDL_GetError());
	}

	return std::unique_ptr<TTF_Font, TTFFontCloser>(font);
}

std::unique_ptr<TTF_Text, TTFTextDestroyer> Renderer::createText(TTF_Font *font, const std::string &text) const
{
	TTF_Text *ttfText = TTF_CreateText(textEngine.get(), font, text.c_str(), 0);
	return std::unique_ptr<TTF_Text, TTFTextDestroyer>(ttfText);
}

void Renderer::startFrame(const int red, const int green, const int blue, const int alpha) const
{
	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);
	SDL_RenderClear(renderer.get());
}

void Renderer::endFrame() const
{
	SDL_RenderPresent(renderer.get());
}

void Renderer::centerCameraOn(const glm::vec2 &position, const glm::vec2 &levelPos, const glm::vec2 &levelSize)
{
	const float newViewportXCoordinate = position.x - (viewport.w / 2.0f);
	const float newViewportYCoordinate = position.y - (viewport.h / 2.0f);

	viewport.x = newViewportXCoordinate;
	viewport.y = newViewportYCoordinate;

	if (levelSize.x <= viewport.w) {
		// Level is smaller than viewport -> center level horizontally
		viewport.x = levelPos.x + (levelSize.x - viewport.w) / 2;
	} else {
		viewport.x = std::clamp(newViewportXCoordinate, levelPos.x, levelSize.x + levelPos.x - viewport.w);
	}

	if (levelSize.y <= viewport.h) {
		// Level is smaller than viewport -> center level vertically
		viewport.y = levelPos.y + (levelSize.y - viewport.h) / 2;
	} else {
		viewport.y = std::clamp(newViewportYCoordinate, levelPos.y, levelSize.y + levelPos.y - viewport.h);
	}

	// SDL_Log("Center camera on (%f) with level of (pos,size):(%f,%f), viewport (x,y):(%f,%f)", position.x, levelPos.x,
	// levelSize.x, viewport.x, viewport.y);
}

void Renderer::renderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scrollFactor) const
{
	const float repetitions = 5.0f;
	dstRect.x -= viewport.x * scrollFactor + texture.w;
	dstRect.w = static_cast<float>(texture.w) * repetitions;
	dstRect.y -= viewport.y;

	SDL_RenderTextureTiled(renderer.get(), &texture, nullptr, 1, &dstRect);
}

void Renderer::renderCircularFade(float progress, SDL_FColor color)
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

void Renderer::renderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> srcRect,
                             std::optional<SDL_FRect> dstRect, const bool flip) const
{
	const SDL_FRect *srcPtr = srcRect ? &srcRect.value() : nullptr;
	SDL_FRect localDst;

	SDL_FRect *dstPtr = nullptr;
	if (dstRect) {
		localDst = dstRect.value();
		localDst.x -= viewport.x;
		localDst.y -= viewport.y;
		dstPtr = &localDst;
	}

	const SDL_FlipMode flipMode = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(renderer.get(), &texture, srcPtr, dstPtr, 0, nullptr, flipMode);
}

void Renderer::drawRectShape(const SDL_FRect rect, const SDL_Color fillColor, const SDL_Color outlineColor) const
{

	SDL_SetRenderDrawColor(renderer.get(), fillColor.r, fillColor.g, fillColor.b, fillColor.a);

    SDL_RenderFillRect(renderer.get(), &rect);

	SDL_SetRenderDrawColor(renderer.get(), outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    SDL_RenderRect(renderer.get(), &rect);
}

void Renderer::drawTextAt(TTF_Text* textObject, const float x, const float y, const SDL_Color color) const
{
	TTF_SetTextColor(textObject, color.r, color.g, color.b, color.a);
	TTF_DrawRendererText(textObject, x, y);

}

glm::vec2 Renderer::windowToLogical(float windowX, float windowY) const
{
	float logicalX = 0.0f;
	float logicalY = 0.0f;

	// SDL3 Helper: Converts window (mouse) coordinates to logical space
	SDL_RenderCoordinatesFromWindow(renderer.get(), windowX, windowY, &logicalX, &logicalY);

	return glm::vec2{logicalX, logicalY};
}