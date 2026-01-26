#pragma once
#include "IObjectParser.hpp"

class SpawnPointParser final : public IObjectParser {
  public:
	SpawnPointParser();

	SpawnPointParser(SpawnPointParser &&) = delete;
	SpawnPointParser(const SpawnPointParser &) = delete;
	SpawnPointParser &operator=(const SpawnPointParser &) = delete;
	SpawnPointParser &operator=(SpawnPointParser &&) = delete;
	~SpawnPointParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
};
