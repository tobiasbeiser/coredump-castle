#pragma once
#include <functional>
#include <memory>
#include <string_view>
#include <glm/vec2.hpp>

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"


namespace Ui
{
    class Label;

    class Button final : public Widget {
    public:
        Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick);
        Button(std::unique_ptr<Widget> customContent, std::function<void()> onClick);
        void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
        bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
        void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;
        void setButtonDisabled(const bool disabled) { isDisabled = disabled;};

    private:
        glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;
        std::unique_ptr<Widget> content;
        Label *textLabel = nullptr;
        std::function<void()> callback;

        bool isHovered = false;
        bool isPressed = false;
        bool isDisabled = false;
    };

}
