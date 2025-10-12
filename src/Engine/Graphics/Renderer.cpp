#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdexcept>
#include "Renderer.hpp"
#include "../Game/AnimatedGameObject.hpp"
#include "../DebugSystem.hpp"

Renderer::Renderer(const std::string& title, int w, int h)
	: width(w), height(h)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		throw std::runtime_error(SDL_GetError());
	}

	window.reset(SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE));
	if (!window) throw std::runtime_error(SDL_GetError());

	renderer.reset(SDL_CreateRenderer(window.get(), nullptr));
	if (!renderer) throw std::runtime_error(SDL_GetError());

	SDL_SetRenderLogicalPresentation(renderer.get(), logW, logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_SetRenderVSync(renderer.get(), 1);

}



Renderer::~Renderer() {
	SDL_Quit();
}

void Renderer::Resize(int w, int h)
{
	width = w;
	height = h;
	SDL_SetWindowSize(window.get(), width, height);
}

std::unique_ptr<SDL_Texture, SDLTextureDeleter>
Renderer::LoadTexture(const std::string& filePath)
{
	SDL_Texture* tex = IMG_LoadTexture(renderer.get(), filePath.c_str());
	if (!tex) {
		SDL_Log("Failed to load texture %s: %s", filePath.c_str(), SDL_GetError());
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
	for (auto& debug : DebugSystem::Instance().debugRects) {
		SDL_SetRenderDrawColor(renderer.get(), debug.color.r, debug.color.g, debug.color.b, debug.color.a);
		SDL_RenderRect(renderer.get(), &debug.rect);
	}

	std::string DebugMessage;
	for (std::string message : DebugSystem::Instance().debugMessages) {
		DebugMessage.append(message + "\n");
	}
	SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
	SDL_RenderDebugText(renderer.get(), 5, 5, DebugMessage.c_str());

	SDL_RenderPresent(renderer.get());
}

void Renderer::DrawAnimatedGameObject(const AnimatedGameObject& obj) const
{
	float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * obj.spriteSize : 0;
	float srcY = obj.currentAnimation != -1 ? obj.currentAnimation * obj.spriteSize : 0;

	SDL_FRect src{
		.x = srcX,
		.y = srcY,
		.w = obj.spriteSize,
		.h = obj.spriteSize
	};

	SDL_FRect dst{
		.x = obj.position.x,
		.y = obj.position.y,
		.w = obj.spriteSize,
		.h = obj.spriteSize
	};

	SDL_FlipMode flip = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	RenderTexture(*(obj.texture), src, dst, obj.direction == -1);


	if (DebugSystem::Instance().render.showBoundingBoxes) {
		SDL_SetRenderDrawColor(renderer.get(), 255, 0, 0, 255);
		SDL_FRect box{
			.x = obj.position.x + obj.collisionBox.x,
			.y = obj.position.y + obj.collisionBox.y,
			.w = obj.collisionBox.w,
			.h = obj.collisionBox.h
		};
		SDL_RenderRect(renderer.get(), &box);
	}
}

void Renderer::DrawGameObject(const GameObject& obj, const float srcX, const float srcY, const float dstX, const float dstY) const
{
	SDL_FRect src{
		.x = srcX * obj.spriteSize,
		.y = srcY * obj.spriteSize,
		.w = obj.spriteSize,
		.h = obj.spriteSize
	};

	SDL_FRect dst{
		.x = dstX * obj.spriteSize,
		.y = dstY * obj.spriteSize,
		.w = obj.spriteSize,
		.h = obj.spriteSize
	};
	RenderTexture(*(obj.texture), src, dst, false);
}

void Renderer::CenterCameraOn(const GameObject& gameObject)
{
	viewport.x = (gameObject.position.x + gameObject.spriteSize / 2) - viewport.w / 2;
}

void Renderer::DrawTexture(SDL_Texture& texture, const float srcX, const float srcY, const float dstX, const float dstY, const float spriteSize) const
{
	SDL_FRect src{
	.x = srcX * spriteSize,
	.y = srcY * spriteSize,
	.w = spriteSize,
	.h = spriteSize
	};

	SDL_FRect dst{
		.x = dstX * spriteSize,
		.y = dstY * spriteSize,
		.w = spriteSize,
		.h = spriteSize
	};
	RenderTexture(texture, src, dst, false);
}

void Renderer::DrawBackground(SDL_Texture& texture)const {
	SDL_RenderTexture(renderer.get(), &texture, nullptr, nullptr);
}

void Renderer::DrawParalaxBackground(SDL_Texture& texture, const float& scrollPos, const float y, const float scale = 1)const {
	SDL_FRect dst{ .x = -scrollPos, .y = y, .w = texture.w * 5.0f, .h = static_cast<float>(texture.h) };

	SDL_RenderTextureTiled(renderer.get(), &texture, nullptr, scale, &dst);
}


void Renderer::RenderTexture(SDL_Texture& texture, const SDL_FRect& srcRect, SDL_FRect& dstRect,const bool flip) const
{
	dstRect.x -= viewport.x;
	SDL_FlipMode flipMode = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	SDL_RenderTextureRotated(renderer.get(), &texture, &srcRect, &dstRect, 0, nullptr, flipMode);
}


