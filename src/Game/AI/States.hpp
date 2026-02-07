#pragma once

#include <string>
#include <string_view>

namespace GameEngine::AI {

enum class AIState {
	Patrol,
	Chase,
	Attacking,
	Recover,
	Staggered,
	Fire,
	CombatDecision,
	CheckChargeChance,
	TelegraphCharge,
	Charging,
	Retreat,
	EvadeJump,
	PreJump,
	CorneredFire,
	None
};

constexpr std::string_view toString(const AIState state) noexcept
{
	switch (state) {
	case AIState::Patrol:
		return "patrol";
	case AIState::Chase:
		return "chase";
	case AIState::Attacking:
		return "attacking";
	case AIState::Recover:
		return "recover";
	case AIState::Staggered:
		return "staggered";
	case AIState::Fire:
		return "fire";
	case AIState::CombatDecision:
		return "combat_decision";
	case AIState::CheckChargeChance:
		return "check_charge_chance";
	case AIState::TelegraphCharge:
		return "telegraph_charge";
	case AIState::Charging:
		return "charging";
	case AIState::Retreat:
		return "retreat";
	case AIState::EvadeJump:
		return "evade_jump";
	case AIState::PreJump:
		return "pre_jump";
	case AIState::CorneredFire:
		return "cornered_fire";
	case AIState::None:
	default:
		return "none";
	}
}

} // namespace GameEngine::AI

template <>
struct std::hash<GameEngine::AI::AIState> {
	std::size_t operator()(const GameEngine::AI::AIState &state) const noexcept
	{
		return std::hash<int>{}(static_cast<int>(state));
	}
}; // namespace std
