#include "Renderer.hpp"
#include "Engine/Debug/DebugSystem.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
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

	SDL_SetRenderVSync(renderer.get(), 1);

	if (!TTF_Init()) {
		throw std::runtime_error(SDL_GetError());
	}

	textEngine.reset(TTF_CreateRendererTextEngine(renderer.get()));

	setupImGui();
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

Resolution Renderer::getLogicalResolution() const
{
	return logicalResolution;
}

glm::vec2 Renderer::windowToLogical(const float windowX, const float windowY) const
{
	float logicalX = 0.0f;
	float logicalY = 0.0f;

	// SDL3 Helper: Converts window (mouse) coordinates to logical space
	SDL_RenderCoordinatesFromWindow(renderer.get(), windowX, windowY, &logicalX, &logicalY);

	return glm::vec2{logicalX, logicalY};
}

SDL_Renderer *Renderer::getNativeRenderer() const
{
	return renderer.get();
}

TTF_TextEngine *Renderer::getTextEngine() const
{
	return textEngine.get();
}

std::unique_ptr<SDL_Texture, SDLTextureDeleter> Renderer::loadTexture(const std::filesystem::path &filePath) const
{
	SDL_Texture *tex = IMG_LoadTexture(renderer.get(), filePath.string().c_str());
	if (!tex) {
		throw std::runtime_error(SDL_GetError());
	}
	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

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
	SDL_SetRenderLogicalPresentation(renderer.get(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
	startImGuiFrame();
	SDL_SetRenderLogicalPresentation(renderer.get(), logicalResolution.width, logicalResolution.height,
	                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);
	SDL_RenderClear(renderer.get());
}

void Renderer::endFrame() const
{
	{
		DebugSystem::renderGameOverlay(renderer.get(), viewport);
		DebugSystem::updateSettings(renderer.get());
		DebugSystem::drawUI();
	}

	SDL_SetRenderLogicalPresentation(renderer.get(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);

	{
		PROFILE_SCOPE("Renderer::ImGui");
		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
	}
	SDL_SetRenderLogicalPresentation(renderer.get(), logicalResolution.width, logicalResolution.height,
	                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);
	{
		PROFILE_SCOPE("Renderer::Present");
		SDL_RenderPresent(renderer.get());
	}
}

void Renderer::setCameraCenter(const glm::vec2 &newCenter)
{
	const float newViewportXCoordinate = newCenter.x - (viewport.w / 2.0f);
	const float newViewportYCoordinate = newCenter.y - (viewport.h / 2.0f);

	viewport.x = newViewportXCoordinate;
	viewport.y = newViewportYCoordinate;
}

void Renderer::centerCameraOn(const glm::vec2 &position, const glm::vec2 &levelPos, const glm::vec2 &levelSize)
{
	setCameraCenter(position);

	if (levelSize.x <= viewport.w) {
		// Level is smaller than viewport -> center level horizontally
		viewport.x = levelPos.x + (levelSize.x - viewport.w) / 2;
	} else {
		viewport.x = std::clamp(viewport.x, levelPos.x, levelSize.x + levelPos.x - viewport.w);
	}

	if (levelSize.y <= viewport.h) {
		// Level is smaller than viewport -> center level vertically
		viewport.y = levelPos.y + (levelSize.y - viewport.h) / 2;
	} else {
		viewport.y = std::clamp(viewport.y, levelPos.y, levelSize.y + levelPos.y - viewport.h);
	}
}

void Renderer::renderTextureTiled(SDL_Texture &texture, SDL_FRect &dstRect, const float scrollFactor) const
{
	constexpr float repetitions = 5.0f;
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
	const float maxRadius = std::sqrt(width * width + height * height);
	const float radius = maxRadius * progress;

	constexpr int segments = 64;
	std::vector<SDL_Vertex> vertices(segments + 2);

	vertices[0].position = {width / 2.0f, height / 2.0f};
	vertices[0].color = color;

	for (int i = 0; i <= segments; ++i) {
		const float angle = 2.0f * std::numbers::pi * i / segments;
		const float x = width / 2.0f + radius * cos(angle);
		const float y = height / 2.0f + radius * sin(angle);
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
void Renderer::renderVignette(const float intensity, const SDL_FColor &color,
                              const std::optional<glm::vec2> &focalPoint) const
{
	if (intensity <= 0.0f)
		return;

	const SDL_FColor innerColor = {color.r, color.g, color.b, 0.0f};
	const SDL_FColor outerColor = {color.r, color.g, color.b, std::clamp(intensity, 0.0f, 1.0f)};

	const float w = static_cast<float>(logicalResolution.width);
	const float h = static_cast<float>(logicalResolution.height);

	// 1. Determine Center (Default to screen center if no focalPoint provided)
	const float centerX = focalPoint ? focalPoint->x : w / 2.0f;
	const float centerY = focalPoint ? focalPoint->y : h / 2.0f;

	// 2. Dimensions
	// Inner Radius: The "clear hole" size (e.g., 40% of screen height)
	const float innerRadius = h * 0.4f;

	// Outer Radius: Must extend from the focal point to the FARTHEST corner
	// otherwise you will see the edge of the mesh when the player moves to the side.
	const float distTL = std::hypot(centerX - 0.0f, centerY - 0.0f); // Top-Left
	const float distTR = std::hypot(centerX - w, centerY - 0.0f);    // Top-Right
	const float distBL = std::hypot(centerX - 0.0f, centerY - h);    // Bottom-Left
	const float distBR = std::hypot(centerX - w, centerY - h);       // Bottom-Right

	const float maxDist = std::max({distTL, distTR, distBL, distBR});
	const float outerRadius = maxDist + 50.0f; // Add buffer just in case

	// 3. Generate Geometry
	constexpr int segments = 32;
	std::vector<SDL_Vertex> vertices;
	std::vector<int> indices;

	for (int i = 0; i <= segments; ++i) {
		const float angle = (static_cast<float>(i) / segments) * 2.0f * std::numbers::pi;
		const float cosA = std::cos(angle);
		const float sinA = std::sin(angle);

		vertices.push_back(
		    SDL_Vertex{{centerX + cosA * innerRadius, centerY + sinA * innerRadius}, innerColor, {0, 0}});

		vertices.push_back(
		    SDL_Vertex{{centerX + cosA * outerRadius, centerY + sinA * outerRadius}, outerColor, {0, 0}});
	}

	for (int i = 0; i < segments; ++i) {
		int innerCurrent = i * 2;
		int outerCurrent = i * 2 + 1;
		int innerNext = (i + 1) * 2;
		int outerNext = (i + 1) * 2 + 1;

		indices.push_back(innerCurrent);
		indices.push_back(outerCurrent);
		indices.push_back(innerNext);

		indices.push_back(outerCurrent);
		indices.push_back(outerNext);
		indices.push_back(innerNext);
	}

	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
	SDL_RenderGeometry(renderer.get(), nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}

SDL_FRect Renderer::getViewport() const
{
	return viewport;
}

void Renderer::setupImGui() const
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
	ImGui_ImplSDLRenderer3_Init(renderer.get());
}

void Renderer::startImGuiFrame()
{
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void Renderer::renderTexture(SDL_Texture &texture, const std::optional<SDL_FRect> &srcRect,
                             const std::optional<SDL_FRect> &dstRect, const bool flip, const double angle) const
{
	const SDL_FRect *srcPtr = srcRect ? &srcRect.value() : nullptr;
	SDL_FRect localDst;

	const SDL_FRect *dstPtr = nullptr;

	if (dstRect) {
		localDst = dstRect.value();
		localDst.x -= viewport.x;
		localDst.y -= viewport.y;
		dstPtr = &localDst;
	}

	const SDL_FlipMode flipMode = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(renderer.get(), &texture, srcPtr, dstPtr, angle, nullptr, flipMode);
}

void Renderer::renderAbsolutePositionTexture(SDL_Texture &texture, const std::optional<SDL_FRect> &srcRect,
                                             const std::optional<SDL_FRect> &dstRect, const bool flip,
                                             const double angle) const
{
	const SDL_FRect *srcPtr = srcRect ? &srcRect.value() : nullptr;

	const SDL_FRect *dstPtr = dstRect ? &dstRect.value() : nullptr;

	const SDL_FlipMode flipMode = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(renderer.get(), &texture, srcPtr, dstPtr, angle, nullptr, flipMode);
}

void Renderer::drawRectShape(const SDL_FRect rect, const SDL_Color fillColor, const SDL_Color outlineColor) const
{

	SDL_SetRenderDrawColor(renderer.get(), fillColor.r, fillColor.g, fillColor.b, fillColor.a);

	SDL_RenderFillRect(renderer.get(), &rect);

	SDL_SetRenderDrawColor(renderer.get(), outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
	SDL_RenderRect(renderer.get(), &rect);
}

void Renderer::drawText(TTF_Text *textObject, const float x, const float y) const
{
	TTF_DrawRendererText(textObject, x, y);
}
