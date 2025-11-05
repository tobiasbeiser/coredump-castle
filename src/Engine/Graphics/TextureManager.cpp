#include "TextureManager.hpp"
#include <stdexcept>

SDL_Texture &TextureManager::getTexture(const std::filesystem::path &filePath)
{
	auto iterator = textures.find(filePath);
	if (iterator != textures.end()) {
		return *(iterator->second); // found in cache
	}

	textures[filePath] = std::move(renderer.loadTexture(filePath));
	return *(textures[filePath]);
}
