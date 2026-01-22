#include "FontManager.hpp"

TTF_Font* FontManager::getFont(const std::filesystem::path& filePath, const int fontSize)
{
	const FontKey key{filePath, fontSize};

	if (const auto iterator = fonts.find(key); iterator != fonts.end()) {
		return &*iterator->second;
	}

	fonts[key] = renderer.loadFont(filePath, fontSize);

	return fonts[key].get();
}