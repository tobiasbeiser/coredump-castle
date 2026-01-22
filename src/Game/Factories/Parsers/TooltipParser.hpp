#pragma once
#include "IObjectParser.hpp"

namespace GameEngine::ECS {
class Registry;
}
class TooltipParser final : public IObjectParser {
  public:
	explicit TooltipParser(GameEngine::ECS::Registry &registry);
	TooltipParser(TooltipParser &&) = delete;
	TooltipParser(const TooltipParser &) = delete;
	TooltipParser &operator=(const TooltipParser &) = delete;
	TooltipParser &operator=(TooltipParser &&) = delete;
	~TooltipParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	GameEngine::ECS::Registry &registry;
};
