#include "TextureManager.hpp"
#include <stdexcept>

SDL_Texture& TextureManager::GetTexture(const std::string &filePath)
{
	auto it = textures.find(filePath);
	if (it != textures.end()) {
		return *(it->second); // found in cache
	}

	textures[filePath] = std::move(renderer.LoadTexture(filePath));
	return *(textures[filePath]);
}
