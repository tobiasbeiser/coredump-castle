#pragma once
#include "Game/Factories/TileFactory.hpp"
#include "IObjectParser.hpp"

class KeyParser final : public IObjectParser {
  public:
	explicit KeyParser(GameEngine::ECS::Registry &registry, const TileFactory &tileFactory);
	KeyParser(KeyParser &&) = delete;
	KeyParser(const KeyParser &) = delete;
	KeyParser &operator=(const KeyParser &) = delete;
	KeyParser &operator=(KeyParser &&) = delete;
	~KeyParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	GameEngine::ECS::Registry &registry;
	const TileFactory &tileFactory;
};
