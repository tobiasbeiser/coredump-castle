#pragma once
#include <memory>

#include "Core/UiContext.hpp"

namespace GameEngine {
class GameEngine;
}

namespace CoredumpContext {
std::unique_ptr<Ui::UIContext> getContext(GameEngine::GameEngine &game);

}
