#pragma once
#include "Renderer.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class TextureManager {
  public:
	TextureManager(Renderer &renderer) : renderer(renderer) {};

	SDL_Texture &GetTexture(const std::filesystem::path &filePath);

	void Cleanup() { textures.clear(); }

  private:
	std::unordered_map<std::filesystem::path, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> textures;
	Renderer &renderer;
};