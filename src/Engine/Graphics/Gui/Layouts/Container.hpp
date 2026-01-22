#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"

namespace Ui
{
    class Container : public Widget {
      public:
        void addWidget(std::unique_ptr<Widget> widget);
        bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
        void draw(const UIContext &context, glm::vec2 parentOffset) const override;

      protected:
        [[nodiscard]] const std::vector<std::unique_ptr<Widget>>& getChildren() const;
      private:
        std::vector<std::unique_ptr<Widget>> children;
    };
}
