#pragma once
#include "Game/Factories/TileFactory.hpp"
#include "IObjectParser.hpp"

class SavePointParser final : public IObjectParser {
  public:
	SavePointParser(GameEngine::ECS::Registry &registry, const TileFactory &tileFactory);

	SavePointParser(SavePointParser &&) = delete;
	SavePointParser(const SavePointParser &) = delete;
	SavePointParser &operator=(const SavePointParser &) = delete;
	SavePointParser &operator=(SavePointParser &&) = delete;
	~SavePointParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	GameEngine::ECS::Registry &registry;
	const TileFactory &tileFactory;
};
