#include "TextureManager.hpp"
#include <stdexcept>

SDL_Texture &TextureManager::getTexture(const std::filesystem::path &filePath)
{
	if (const auto iterator = textures.find(filePath); iterator != textures.end()) {
		return *(iterator->second); // found in cache
	}

	textures[filePath] = renderer.loadTexture(filePath);
	return *(textures[filePath]);
}
