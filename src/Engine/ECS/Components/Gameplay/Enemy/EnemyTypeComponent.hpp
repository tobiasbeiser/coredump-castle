#pragma once

enum class EnemyType { Base, Advanced, Long, Boss };

struct EnemyTypeComponent {
	EnemyType type;
};