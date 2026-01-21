#pragma once
#include "Game/Factories/PlayerFactory.hpp"
#include "IObjectParser.hpp"

class PlayerParser final : public IObjectParser {
  public:
	explicit PlayerParser(PlayerFactory &playerFactory);

	PlayerParser(PlayerParser &&) = delete;
	PlayerParser(const PlayerParser &) = delete;
	PlayerParser &operator=(const PlayerParser &) = delete;
	PlayerParser &operator=(PlayerParser &&) = delete;
	~PlayerParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	PlayerFactory &playerFactory;
};
