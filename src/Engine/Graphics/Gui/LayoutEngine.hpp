#pragma once
#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>
#include <optional>
#include <span>
#include <vector>

namespace Ui {
class Widget;
struct UIContext;
} // namespace Ui

namespace Ui::LayoutEngine {
static constexpr int framePixelOffset = 4;

enum class Orientation { Vertical, Horizontal };
enum class Alignment { Start, Center, End, Stretch };

struct AnchorParams {
	glm::vec2 anchor = {0.0f, 0.0f};
	glm::vec2 pivot = {0.0f, 0.0f};
	glm::vec2 offset = {0.0f, 0.0f};

	static const AnchorParams center;
	static const AnchorParams topLeft;
	static const AnchorParams topRight;
	static const AnchorParams topCenter;
	static const AnchorParams bottomCenter;
};

struct LayoutProps {
	int row = 0;
	int col = 0;
	int spanX = 1;
	int spanY = 1;

	// Linear/Stack
	float weight = 0.0f;

	Alignment hAlign = Alignment::Center;
	Alignment vAlign = Alignment::Center;

	glm::vec2 minSize{0, 0};

	// If set, this overrides hAlign/vAlign logic
	std::optional<AnchorParams> anchor = std::nullopt;
};

struct LayoutItem : LayoutProps {
	Widget *widget = nullptr;
	glm::vec2 idealSize{0, 0};

	SDL_FRect calculatedBounds{0, 0, 0, 0};
};
struct Padding {
	float left = 0.f;
	float top = 0.f;
	float right = 0.f;
	float bottom = 0.f;

	static Padding all(const float v) { return {v, v, v, v}; }
	static Padding symmetric(const float h, const float v) { return {h, v, h, v}; }
};

struct LinearConfig {
	Orientation orientation = Orientation::Vertical;
	float spacing = 0.0f;
	Padding padding;
};

struct Track {
	enum Type { Fixed, Flex } type = Flex;
	float value = 1.0f; // Pixels for Fixed, Weight for Flex

	static Track fix(const float px) { return {Fixed, px}; }
	static Track flex(const float w) { return {Flex, w}; }
};

struct GridConfig {
	std::vector<Track> colDefs;
	std::vector<Track> rowDefs;

	int uniformCols = 1; // Fallback
	int uniformRows = 1; // Fallback

	float gap = 0.0f;
	Padding padding;
};

SDL_FRect positionAnchored(glm::vec2 allocatedSize, glm::vec2 ownSize, const AnchorParams &anchorParams);
SDL_FRect positionCentered(glm::vec2 allocatedSize, glm::vec2 ownSize);
SDL_FRect positionFrameChild(glm::vec2 frameSize, int outlineOffset = framePixelOffset);
SDL_FRect positionFrame(SDL_FRect contentBounds, int outlineOffset = framePixelOffset);
void solveLinear(std::span<LayoutItem> items, const SDL_FRect &parentRect, const LinearConfig &config);
void solveStack(std::span<LayoutItem> items, const SDL_FRect &parentRect);
void solveGrid(std::span<LayoutItem> items, const SDL_FRect &parentRect, const GridConfig &cfg);

} // namespace Ui::LayoutEngine