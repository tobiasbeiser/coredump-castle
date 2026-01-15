#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <limits>
#include <memory>
#include <vector>

// Use float limits for "Infinite" space
constexpr float kInfinity = std::numeric_limits<float>::infinity();

// Defines the constraints passed from Parent to Child during layout.
struct BoxConstraints {
	float minWidth = 0.0f;
	float maxWidth = kInfinity;
	float minHeight = 0.0f;
	float maxHeight = kInfinity;

	// Helper: Create tight constraints (force specific size)
	static BoxConstraints tight(const float width, const float height) { return {width, width, height, height}; }

	// Helper: Loosen constraints (0 to max)
	static BoxConstraints loose(const glm::vec2 size) { return {0.0f, size.x, 0.0f, size.y}; }

	[[nodiscard]] glm::vec2 constrain(const glm::vec2 size) const
	{
		return {std::clamp(size.x, minWidth, maxWidth), std::clamp(size.y, minHeight, maxHeight)};
	}

	[[nodiscard]] bool isTight() const { return minWidth == maxWidth && minHeight == maxHeight; }
	[[nodiscard]] bool hasBoundedWith() const { return maxWidth < kInfinity; }
	[[nodiscard]] bool hasBoundedHeight() const { return maxHeight < kInfinity; }
};

// Controls how a child aligns within the parent's cross-axis
enum class CrossAxisAlignment {
	Start,
	Center,
	End,
	Stretch // Force child to fill cross-axis
};
enum class WidgetState : uint8_t { Normal, Hovered, Pressed, Disabled };

enum class SizeMode {
	Fixed,       // Use explicit width/height
	WrapContent, // Shrink to fit content (e.g., text size)
	MatchParent  // Expand to fill available parent space
};

enum class Orientation {
	Vertical,
	Horizontal,
};

struct Paint {
	SDL_Color fillCol{};
	SDL_Color outlineCol{};
	SDL_Color textCol{};
};

struct ButtonStyle {
	float widthPadding = 5.0f;
	float heightPadding = 5.0f;
	Paint normal;
	Paint hovered;
	Paint pressed;
	Paint disabled;

	[[nodiscard]] const Paint &paintFor(const WidgetState s) const noexcept
	{
		switch (s) { // TODO: add disabled state, e.g. if we don't have previous saves for the "Continue" view
		case WidgetState::Hovered:
			return hovered;
		case WidgetState::Pressed:
			return pressed;
		case WidgetState::Disabled:
			return disabled;
		case WidgetState::Normal:
		default:
			return normal;
		}
	}
};

struct FrameStyle {
	// TODO: rework frame default to allow for individual colors
	SDL_Color fill{0, 0, 0, 0};
	SDL_Color outline{0, 0, 0, 0};
	float padding = 0.0f; // could be used by layout later
};

struct UiTheme {
	SDL_Color titleTextColor;
	ButtonStyle button;
	FrameStyle frame;
};

struct UIContext {
	Renderer &renderer;
	TTF_Font *titleFont = nullptr;
	TTF_Font *bigFont = nullptr;
	TTF_Font *textFont = nullptr;
	UiTheme theme;
	Resolution resolution;
};

class Widget {
  public:
	Widget(const Widget &) = delete;
	Widget &operator=(const Widget &) = delete;
	Widget(Widget &&) = delete;
	Widget &operator=(Widget &&) = delete;
	virtual ~Widget() = default;

	glm::vec2 measure(const UIContext &ctx, BoxConstraints constraints)
	{
		if (widthMode == SizeMode::Fixed) {
			constraints.minWidth = fixedSize.x;
			constraints.maxWidth = fixedSize.x;
		}

		if (heightMode == SizeMode::Fixed) {
			constraints.minHeight = fixedSize.y;
			constraints.maxHeight = fixedSize.y;
		}

		if (cachedSize.has_value() && lastConstraints.has_value() && constraints.minWidth == lastConstraints->minWidth
		    && constraints.maxWidth == lastConstraints->maxWidth && constraints.minHeight == lastConstraints->minHeight
		    && constraints.maxHeight == lastConstraints->maxHeight && !isDirty) {
			return cachedSize.value();
		}

		glm::vec2 size = performMeasure(ctx, constraints);
		size = constraints.constrain(size);

		cachedSize = size;
		lastConstraints = constraints;
		isDirty = false;
		return size;
	}

	[[nodiscard]] glm::vec2 getMeasuredSize() const { return cachedSize.value_or(glm::vec2{0.0f, 0.0f}); }

	virtual void layout(const UIContext &ctx, const SDL_FRect allocatedRect) { bounds = allocatedRect; }

	virtual bool handleEvent(glm::vec2 mousePos, const SDL_Event &event) = 0;
	virtual void draw(const UIContext &ctx) = 0;

	void invalidate() { isDirty = true; }

	void setLayoutWeight(const float weight) { layoutWeight = weight; }
	[[nodiscard]] float getLayoutWeight() const { return layoutWeight; }

	void setSizeMode(const SizeMode w, const SizeMode h)
	{
		widthMode = w;
		heightMode = h;
		invalidate();
	}
	void setFixedSize(const glm::vec2 size)
	{
		fixedSize = size;
		widthMode = SizeMode::Fixed;
		heightMode = SizeMode::Fixed;
	}

	[[nodiscard]] SDL_FRect getBounds() const { return bounds; }
	[[nodiscard]] SizeMode getWidthMode() const { return widthMode; }
	[[nodiscard]] SizeMode getHeightMode() const { return heightMode; }
	[[nodiscard]] glm::vec2 getAbsolutePosition() const { return {bounds.x, bounds.y}; }

  protected:
	Widget() = default;
	[[nodiscard]] SDL_FRect getAbsoluteRect() const noexcept { return bounds; }

	virtual glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) = 0;

  private:
	SDL_FRect bounds = {0.0f, 0.0f, 0.0f, 0.0f}; // Position is relative to Parent

	float layoutWeight = 0.0f;
	SizeMode widthMode = SizeMode::WrapContent;
	SizeMode heightMode = SizeMode::WrapContent;
	glm::vec2 fixedSize = {0.0f, 0.0f};
	std::optional<glm::vec2> cachedSize;
	std::optional<BoxConstraints> lastConstraints;
	bool isDirty = true;
};

class Spacer : public Widget {
  public:
	explicit Spacer(const float width = 0.f, const float height = 0.f) : width(width), height(height) {}

	bool handleEvent(glm::vec2 mousePos, const SDL_Event &) override { return false; }
	void draw(const UIContext &) override {} // Invisible

  protected:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) override { return {width, height}; }

  private:
	float width;
	float height;
};

// A simple text label
class Label : public Widget {
  public:
	Label(const std::string_view labelText, TTF_Font *font) : labelText(labelText), font(font)
	{
		setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
	}

	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 mousePos, const SDL_Event &) override { return false; }
	void draw(const UIContext &ctx) override;

	void setColor(const SDL_Color newColor) { color = newColor; }

  private:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) override;
	std::string labelText;
	std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
	TTF_Font *font;
	SDL_Color color{255, 255, 255, 255}; // white default
};

class Button final : public Widget {
  public:
	Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick);
	Button(std::unique_ptr<Widget> customContent, std::function<void()> onClick);
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 mousePos, const SDL_Event &event) override;
	void draw(const UIContext &ctx) override;
	void setButtonDisabled(const bool disabled);

private:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) override;
	std::unique_ptr<Widget> content;
	std::function<void()> callback;

	bool isHovered = false;
	bool isPressed = false;
	bool isDisabled = false;
};

class Container : public Widget {
  public:
	void addWidget(std::unique_ptr<Widget> widget) { children.push_back(std::move(widget)); }
	bool handleEvent(glm::vec2 mousePos, const SDL_Event &event) override;
	void draw(const UIContext &context) override;

  protected:
	std::vector<std::unique_ptr<Widget>> children;
};

class LinearLayout : public Container {
  public:
	explicit LinearLayout(const Orientation orient = Orientation::Vertical, const float gap = 5.0f)
	    : orientation(orient), spacing(gap)
	{
		setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent);
	}

	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	void setOrientation(const Orientation newOrientation) { orientation = newOrientation; }
	void setCrossAxisAlignment(CrossAxisAlignment align);
	void setSpacing(const float gap)
	{ // TODO: change return type to LinearLayout& (return *this;) after CRTP implementation
		spacing = gap;
	}

  protected:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) override;

  private:
	Orientation orientation;
	float spacing;
	CrossAxisAlignment crossAxisAlignment = CrossAxisAlignment::Center;
};

class GuiManager {
  public:
	void setRoot(std::unique_ptr<Widget> root) { rootWidget = std::move(root); }

	void updateLayout(const UIContext &context, float screenWidth, float screenHeight) const;

	void handleEvent(const UIContext &ctx, const SDL_Event &event) const;
	void render(const UIContext &ctx) const;

  private:
	std::unique_ptr<Widget> rootWidget;
};