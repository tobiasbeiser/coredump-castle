#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <functional>
#include <memory>
#include <vector>

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
	float padding = 5.0f;
	Paint normal;
	Paint hovered;
	Paint pressed;

	[[nodiscard]] const Paint &paintFor(const WidgetState s) const noexcept
	{
		switch (s) { //TODO: add disabled state, e.g. if we don't have previous saves for the "Continue" view
		case WidgetState::Hovered:
			return hovered;
		case WidgetState::Pressed:
			return pressed;
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
	TTF_Font *titleFont=nullptr;
	TTF_Font *bigFont=nullptr;
	TTF_Font *textFont=nullptr;
	UiTheme theme;
	Resolution resolution;
};

class Widget {
  public:
	Widget(const Widget &) = delete;
	Widget &operator=(const Widget &) = delete;
	Widget(Widget&&) = delete;
	Widget &operator=(Widget &&) = delete;
	virtual ~Widget() = default;

	virtual glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) = 0;
	virtual void layout(const UIContext &ctx, const SDL_FRect allocatedRect) { bounds = allocatedRect; }

	virtual bool handleEvent(glm::vec2 mousePos, glm::vec2 parentPos, const SDL_Event &event) = 0;
	virtual void draw(const UIContext &ctx, glm::vec2 parentPos) = 0;

	void setSizeMode(const SizeMode w, const SizeMode h)
	{
		widthMode = w;
		heightMode = h;
	}
	void setFixedSize(const glm::vec2 size) { fixedSize = size; }

	[[nodiscard]] SizeMode getWidthMode() const { return widthMode; }
	[[nodiscard]] SizeMode getHeightMode() const { return heightMode; }
	[[nodiscard]] glm::vec2 getRelativePosition() const { return {bounds.x, bounds.y}; }
	[[nodiscard]] glm::vec2 absolutePosition(const glm::vec2 parentAbsPos) const
	{
		return {parentAbsPos.x + bounds.x, parentAbsPos.y + bounds.y};
	}

protected:
	Widget() = default;
	[[nodiscard]] SDL_FRect getAbsoluteRect(const glm::vec2 parentAbsPos) const noexcept
	{
		return SDL_FRect{parentAbsPos.x + bounds.x, parentAbsPos.y + bounds.y, bounds.w, bounds.h};
	}
	// Helper: Resolve final size based on modes
	[[nodiscard]] glm::vec2 resolveSize(const glm::vec2 contentSize, const glm::vec2 availableSpace) const
	{
		float w = (widthMode == SizeMode::Fixed)         ? fixedSize.x
				  : (widthMode == SizeMode::MatchParent) ? availableSpace.x
														 : contentSize.x; // WrapContent

		float h = (heightMode == SizeMode::Fixed)         ? fixedSize.y
				  : (heightMode == SizeMode::MatchParent) ? availableSpace.y
														  : contentSize.y; // WrapContent
		return {w, h};
	}

  private:
	SDL_FRect bounds = {0.0f, 0.0f, 0.0f, 0.0f}; // Position is relative to Parent

	SizeMode widthMode = SizeMode::Fixed;
	SizeMode heightMode = SizeMode::Fixed;
	glm::vec2 fixedSize = {100.0f, 50.0f}; // Necessary for resolving fixed size, gets transferred to bounds in layout()

};

class Spacer : public Widget {
  public:
	explicit Spacer(const float width = 0.f, const float height = 0.f) : width(width), height(height) {}

	glm::vec2 getPreferredSize(const UIContext &, glm::vec2) override { return {width, height}; }
	bool handleEvent(glm::vec2, glm::vec2, const SDL_Event &) override { return false; }
	void draw(const UIContext &, glm::vec2) override {} // Invisible

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

	glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) override;
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2, glm::vec2, const SDL_Event &) override { return false; }
	void draw(const UIContext &ctx, glm::vec2 parentPos) override;

	void setColor(const SDL_Color newColor) { color = newColor; }

  private:
	std::string labelText;
	std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
	TTF_Font *font;
	SDL_Color color{255, 255, 255, 255}; // white default
};

class Button final : public Widget {
  public:
	Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick);
	Button(std::unique_ptr<Widget> customContent, std::function<void()> onClick);
	glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) override;
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 mousePos, glm::vec2 parentAbsPos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentAbsPos) override;

  private:
	std::unique_ptr<Widget> content;
	std::function<void()> callback;

	bool isHovered = false;
	bool isPressed = false;
};


class Container : public Widget {
  public:
	void addWidget(std::unique_ptr<Widget> widget) { children.push_back(std::move(widget)); }
	bool handleEvent(glm::vec2 mousePos, glm::vec2 parentAbsPos, const SDL_Event &event) override;
	void draw(const UIContext &context, glm::vec2 parentAbsPos) override;

  protected:
	std::vector<std::unique_ptr<Widget>> children;
};

class LinearLayout : public Container {
public:
	explicit LinearLayout(const float spacing = 5.0f, const float padding = 5.0f) : spacing(spacing), padding(padding)
	{
		setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent);
	}

	glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) override;
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	void setOrientation(const Orientation newOrientation) {orientation = newOrientation;}

private:
	float spacing;
	float padding;
	Orientation orientation = Orientation::Vertical;
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