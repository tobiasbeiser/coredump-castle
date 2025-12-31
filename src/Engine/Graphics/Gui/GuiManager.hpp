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
	virtual ~Widget() = default;

	virtual glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) = 0;
	virtual void layout(const UIContext &ctx, const SDL_FRect allocatedRect)
	{
		// TODO: add alignment
		bounds = allocatedRect;
	}

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
	[[nodiscard]] SDL_FRect getBounds() const { return bounds; }
	[[nodiscard]] glm::vec2 getRelativePosition() const { return {bounds.x, bounds.y}; }
	[[nodiscard]] glm::vec2 absolutePosition(const glm::vec2 parentAbsPos) const
	{
		return {parentAbsPos.x + bounds.x, parentAbsPos.y + bounds.y};
	}

protected:
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
	glm::vec2 fixedSize = {100.0f, 50.0f};

};

// A simple widget that just consumes space (pushes things apart)
class Spacer : public Widget {
  public:
	explicit Spacer(const float height) : height(height) {}

	glm::vec2 getPreferredSize(const UIContext &, glm::vec2) override { return {0.0f, height}; }
	bool handleEvent(glm::vec2, glm::vec2, const SDL_Event &) override { return false; }
	void draw(const UIContext &, glm::vec2) override {} // Invisible

  private:
	float height;
};

// A simple text label
class Label : public Widget {
  public:
	Label(const std::string_view labelText, TTF_Font *font) : labelText(labelText), font(font) {}

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

class Frame : public Widget {
  public:
	void addWidget(std::unique_ptr<Widget> widget);
	bool handleEvent(glm::vec2 mousePos, glm::vec2 parentPos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentAbsPos) override;

  private:
	std::vector<std::unique_ptr<Widget>> widgets;
};

// Automatically stacks children vertically
class VBox : public Widget {
  public:
	explicit VBox(const float spacing = 5.0f, const float padding = 5.0f) : spacing(spacing), padding(padding)
	{
		// Default VBox behavior: Wrap around children, but width matches parent if needed
		setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
	}

	void addWidget(std::unique_ptr<Widget> widget) { children.push_back(std::move(widget)); }
	glm::vec2 getPreferredSize(const UIContext &ctx, glm::vec2 availableSize) override;
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 mousePos, glm::vec2 parentAbsPos, const SDL_Event &event) override;
	void draw(const UIContext &context, glm::vec2 parentAbsPos) override;

  private:
	std::vector<std::unique_ptr<Widget>> children;
	float spacing;
	float padding;
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
;