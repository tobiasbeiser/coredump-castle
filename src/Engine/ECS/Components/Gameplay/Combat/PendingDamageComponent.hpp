#pragma once

enum class DamageSource { Enemy, Hazard };

struct PendingDamageComponent {
	int damage{0};
	DamageSource source{DamageSource::Enemy};
};