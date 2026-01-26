#pragma once

#include <algorithm>
#include <vector>

enum class AbilityType : int {
	None = 0,
	DoubleJump = 1,
	Slide = 2,
	Dash = 3,
};

struct ProgressComponent {
	std::vector<int> defeatedBosses{};
	std::vector<int> unlockedAbilities{};

	[[nodiscard]] bool hasAbility(const AbilityType ability) const
	{
		const int value = static_cast<int>(ability);
		return std::ranges::find(unlockedAbilities, value) != unlockedAbilities.end();
	}

	void unlockAbility(const AbilityType ability)
	{
		if (const int value = static_cast<int>(ability);
		    std::ranges::find(unlockedAbilities, value) == unlockedAbilities.end()) {
			unlockedAbilities.push_back(value);
		}
	}

	[[nodiscard]] bool isBossDefeated(const int bossId) const
	{
		return std::ranges::find(defeatedBosses, bossId) != defeatedBosses.end();
	}

	void markBossDefeated(const int bossId)
	{
		if (std::ranges::find(defeatedBosses, bossId) == defeatedBosses.end()) {
			defeatedBosses.push_back(bossId);
		}
	}
};
