#pragma once
#include "Game/Factories/EnemyFactory.hpp"
#include "IObjectParser.hpp"

class EnemyParser final : public IObjectParser {
  public:
	explicit EnemyParser(EnemyFactory &enemyFactory);

	EnemyParser(EnemyParser &&) = delete;
	EnemyParser(const EnemyParser &) = delete;
	EnemyParser &operator=(const EnemyParser &) = delete;
	EnemyParser &operator=(EnemyParser &&) = delete;
	~EnemyParser() override = default;

	bool canParse(tson::Object &obj) const override;
	void parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const override;

  private:
	EnemyFactory &enemyFactory;
};
