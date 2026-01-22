#pragma once
#include "Engine/Input/InputManager.hpp"
#include "IObjectParser.hpp"

namespace GameEngine::ECS {
class Registry;
}
class TooltipParser final : public IObjectParser {
  public:
	explicit TooltipParser(GameEngine::ECS::Registry &registry, const InputManager &inputManager);
	TooltipParser(TooltipParser &&) = delete;
	TooltipParser(const TooltipParser &) = delete;
	TooltipParser &operator=(const TooltipParser &) = delete;
	TooltipParser &operator=(TooltipParser &&) = delete;
	~TooltipParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	[[nodiscard]] std::string getTooltipText(const std::string &name) const;
	GameEngine::ECS::Registry &registry;
	const InputManager inputManager;
};
