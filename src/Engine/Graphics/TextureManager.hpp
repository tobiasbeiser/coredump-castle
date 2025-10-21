#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "Renderer.hpp"

class TextureManager {
  public:
	TextureManager(Renderer &renderer) : renderer(renderer) {};

	SDL_Texture& GetTexture(const std::string &filePath);

	void Cleanup() { textures.clear(); }

  private:
	std::unordered_map<std::string, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> textures;
	Renderer &renderer;
};