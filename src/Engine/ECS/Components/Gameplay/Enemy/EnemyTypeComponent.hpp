#pragma once

enum class EnemyType { Base, Long, Advanced, Boss };

struct EnemyTypeComponent {
	EnemyType type;
};