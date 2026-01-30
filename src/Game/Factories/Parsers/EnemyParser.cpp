#include "EnemyParser.hpp"

EnemyParser::EnemyParser(GameEngine::ECS::EnemyFactory &enemyFactory, std::vector<EnemySpawner> &spawners)
    : enemyFactory(enemyFactory), spawners(spawners)
{
}
bool EnemyParser::canParse(tson::Object &obj) const
{
	std::string name = obj.getName();
	std::ranges::transform(name, name.begin(), [](const unsigned char c) { return std::tolower(c); });
	return name.find("enemy") != std::string::npos || name.find("boss") != std::string::npos;
}
void EnemyParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	spawners.push_back(enemyFactory.createEnemy(obj, levelPos, layerIndex));
}