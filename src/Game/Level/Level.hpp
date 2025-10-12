#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <tileson.hpp>

#include "../GameObject.hpp"
#include "../Entities/Player.hpp"

class Level : public GameObject {
public:
	Level(TextureManager& textureManager, Renderer& renderer);

	void Load(const std::string& levelFile, Player& player);
	void Update(const float deltaTime, const float stepSize);
	void Render();
	void RenderForeground() const;

	const std::unique_ptr<tson::Map>& GetMap() const { return map; }

private:

	void RenderTiles(SDL_Texture& texture,const tson::Layer& layer) const;
	void RenderBackground();


	std::unique_ptr<tson::Map> map = nullptr;
	tson::Tileson t;
	glm::ivec2 dimensions;
	std::shared_ptr<SDL_Texture> tileSetTexture = nullptr;
	std::shared_ptr<SDL_Texture> parallaxTexture = nullptr;
	std::shared_ptr<SDL_Texture> foregroundTexture = nullptr;
	TextureManager& textureManager;
	Player* player = nullptr;
	float scrollPos = 0.0f;
};