#pragma once

enum Layer { Background = 0, Parallax = 1, Tiles = 2, Entities = 3, Player = 4, Foreground = 5};

struct RenderComponent {
	Layer layer = Layer::Tiles;
	bool flipX = false;
};