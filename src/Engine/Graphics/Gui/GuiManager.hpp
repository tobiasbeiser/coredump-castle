#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>

class UiElement {
public:
    virtual ~UiElement() = default;
    virtual void handleEvent(float logicalX, float logicalY, const SDL_Event& event) = 0;
    virtual void render(const Renderer& renderer) = 0;
};

class Button : public UiElement {
public:
    Button(const Renderer& renderer, TTF_Font* font, const std::string& label,
           float y, const std::function<void()>& onClick);

    void handleEvent(float logicalX, float logicalY, const SDL_Event& event) override;
    void render(const Renderer &renderer) override;

private:
    SDL_FRect rect;
    std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
    std::function<void()> callback;
    
    // State
    bool isHovered = false;
    bool isPressed = false;
};

class GuiManager {
public:
    explicit GuiManager(Renderer& renderer, TTF_Font* font);

    void createButton(const std::string& label, float y, std::function<void()> onClick);
    void handleEvents(const SDL_Event& event) const;
    void render() const;
private:
    Renderer &renderer;
    TTF_Font* font;

    std::vector<std::unique_ptr<UiElement>> guiElements;
};