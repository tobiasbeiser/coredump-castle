#include "GuiManager.hpp"

#include "Engine/Graphics/Renderer.hpp"

Button::Button(const Renderer& renderer, TTF_Font* font, const std::string& label,
               const float y, const std::function<void()>& onClick)
    : callback(onClick)
{
    //TODO: expose x-positioning, width and height outside
    auto [width, height] = renderer.getLogicalResolution();
    constexpr float WIDTH = 200.0f;
    constexpr float HEIGHT = 40.0f;

    // Center horizontally
    const float x = (width - WIDTH) / 2.0f;
    rect = { x, y, WIDTH, HEIGHT };

    textObject = renderer.createText(font, label);
}

void Button::handleEvent(const float logicalX, const float logicalY, const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        const SDL_Point mouse = { static_cast<int>(logicalX), static_cast<int>(logicalY) };
        const SDL_Rect r = { static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h) };
        isHovered = SDL_PointInRect(&mouse, &r);
    }
    
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (isHovered) {
            isPressed = true;
        }
    }
    
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        if (isHovered && isPressed && callback) {
            callback();
        }
        isPressed = false;
    }

}

void Button::render(const Renderer& renderer)
{
    //TODO: move colors somewhere else?
    SDL_Color fill, outline, textCol;

    if (isPressed) {
        fill = {100, 100, 100, 255};
        outline = {255, 255, 255, 255};
        textCol = {200, 200, 200, 255};
    } else if (isHovered) {
        fill = {50, 50, 150, 255};
        outline = {255, 255, 0, 255};
        textCol = {255, 255, 255, 255};
    } else {
        fill = {30, 30, 30, 255};
        outline = {100, 100, 100, 255};
        textCol = {180, 180, 180, 255};
    }

    renderer.drawRectShape(rect, fill, outline);

    if (textObject) {
        renderer.drawTextCentered(textObject.get(), rect.x, rect.y, rect.w, rect.h, textCol);
    }
}

GuiManager::GuiManager(Renderer &renderer, TTF_Font* font) : renderer(renderer), font(font) {}


void GuiManager::createButton(const std::string& label, float y, std::function<void()> onClick) {

    auto btn = std::make_unique<Button>(renderer, font, label, y, onClick);
    guiElements.push_back(std::move(btn));
}

void GuiManager::handleEvents(const SDL_Event& event) const
{
    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        float mx = 0, my = 0;

        // Extract raw Window coordinates based on event type
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            mx = event.motion.x;
            my = event.motion.y;
        } else {
            mx = event.button.x;
            my = event.button.y;
        }

        // CONVERT to Logical Coordinates
        const glm::vec2 logicalPos = renderer.windowToLogical(mx, my);

        for (auto& el : guiElements) {
            el->handleEvent(logicalPos.x, logicalPos.y, event);
        }
    }
}

void GuiManager::render() const
{
    for (auto& el : guiElements) el->render(renderer);
}
