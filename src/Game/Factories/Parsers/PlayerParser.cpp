#include "PlayerParser.hpp"

PlayerParser::PlayerParser(PlayerFactory &playerFactory) : playerFactory(playerFactory) {}

bool PlayerParser::canParse(tson::Object &obj) const
{
	return obj.getName() == "Player";
}
void PlayerParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	playerFactory.createPlayer(obj, levelPos);
}