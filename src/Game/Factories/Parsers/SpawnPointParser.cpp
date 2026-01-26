#include "SpawnPointParser.hpp"

#include "Engine/Debug/DebugSystem.hpp"

SpawnPointParser::SpawnPointParser() = default;

bool SpawnPointParser::canParse(tson::Object &obj) const
{
	return obj.getName() == "SpawnPoint";
}

void SpawnPointParser::parse(tson::Object &obj, const glm::vec2 &levelPos, int layerIndex) const
{
	const glm::vec2 spawn = {static_cast<float>(obj.getPosition().x) + levelPos.x,
	                         static_cast<float>(obj.getPosition().y) + levelPos.y};
	DebugSystem::instance().addSpawnPoint(spawn);
}
