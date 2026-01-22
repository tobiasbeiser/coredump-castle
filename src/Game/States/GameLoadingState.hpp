#pragma once
#include <functional>
#include <memory>

#include "Engine/GameState.hpp"

namespace GameEngine
{
    class GameLoadingState : public GameState
    {
    public:
        using StateFactory = std::function<std::unique_ptr<GameState>()>;

        GameLoadingState(StateFactory factory, Ui::GuiManager& guiManager)
            : nextStateFactory(std::move(factory)), guiManager(guiManager)
        {
        }

        void init(GameEngine& game) override
        {
        }

        void cleanup() override
        {
        }

        void pause() override
        {
        }

        void resume() override
        {
        }

        void handleEvents(GameEngine& game, const SDL_Event& event) override
        {
        }

        void update(GameEngine& game, float deltaTime) override;
        void render(GameEngine& game, float alpha) override;

    private:
        StateFactory nextStateFactory;
        Ui::GuiManager& guiManager;
        bool hasRenderedOnce = false;
    };
} // namespace GameEngine