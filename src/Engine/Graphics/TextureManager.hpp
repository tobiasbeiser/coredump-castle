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
	TextureManager(Renderer &renderer) : renderer(renderer){};
	TextureManager(const TextureManager &) = delete;
	TextureManager &operator=(const TextureManager &) = delete;
	TextureManager(TextureManager &&) = delete;
	TextureManager &operator=(TextureManager &&) = delete;
	~TextureManager() = default;

	SDL_Texture &getTexture(const std::filesystem::path &filePath);

	void cleanup() { textures.clear(); }

  private:
	std::unordered_map<std::filesystem::path, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> textures;
	Renderer &renderer;
};