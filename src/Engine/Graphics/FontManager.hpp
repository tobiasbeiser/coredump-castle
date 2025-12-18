#pragma once

#include "Renderer.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

struct FontKey {
	std::filesystem::path path;
	int fontSize;

	bool operator==(const FontKey &other) const noexcept { return fontSize == other.fontSize && path == other.path; }
};

// Implementation based on https://www.geeksforgeeks.org/cpp/how-to-create-an-unordered_map-of-pairs-in-c/
struct FontKeyHash {
	std::size_t operator()(const FontKey &k) const noexcept {
		const size_t hash1 = std::hash<std::filesystem::path>{}(k.path);

		const size_t hash2 = std::hash<int>{}(k.fontSize);

		return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
	}
};

class FontManager {
  public:
	explicit FontManager(Renderer &renderer) : renderer(renderer){};
	FontManager(const FontManager &) = delete;
	FontManager &operator=(const FontManager &) = delete;
	FontManager(FontManager &&) = delete;
	FontManager &operator=(FontManager &&) = delete;
	~FontManager() = default;

	[[nodiscard]] TTF_Font* getFont(const std::filesystem::path& filePath, int fontSize);

	void cleanup() { fonts.clear(); }

  private:
	std::unordered_map<FontKey, std::unique_ptr<TTF_Font, TTFFontCloser>, FontKeyHash> fonts;
	Renderer &renderer;
};