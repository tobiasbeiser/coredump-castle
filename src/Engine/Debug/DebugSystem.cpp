#include "DebugSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Profiler.hpp"

#include <imgui.h>
#include <numeric>

void DebugSystem::debug(const std::string &message)
{
	instance().debugMessages.push_back(message);
}

void DebugSystem::addRect(const SDL_FRect &rect, const SDL_Color &color)
{
	instance().debugRects.push_back({rect, color});
}

void DebugSystem::addLine(const glm::vec2 &start, const glm::vec2 &end, const SDL_Color &color)
{
	instance().debugLines.push_back({start, end, color});
}

void DebugSystem::addRaycast(const glm::vec2 &start, const glm::vec2 &end, bool obstructed)
{
	DebugSystem &sys = instance();
	if (!sys.enabled || !sys.render.showRaycasts)
		return;

	const SDL_Color color = obstructed ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 255, 0, 255};
	addLine(start, end, color);
}

void DebugSystem::clear()
{
	instance().debugRects.clear();
	instance().debugLines.clear();
	instance().debugMessages.clear();
}

void DebugSystem::renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport)
{
	PROFILE_SCOPE("Debug::Debug Overlay");

	DebugSystem &sys = instance();
	if (sys.render.showBoundingBoxes)
		sys.drawCollisionBoxes();

	if (sys.render.showHitBoxes)
		sys.drawHitBoxes();

	if (sys.render.showTooltipBoxes)
		sys.drawTooltipBoxes();

	for (const auto &[rect, color] : sys.debugRects) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		SDL_FRect drawRect = rect;
		drawRect.x -= viewport.x;
		drawRect.y -= viewport.y;

		SDL_RenderRect(renderer, &drawRect);
	}

	if (sys.render.showRaycasts) {
		for (const auto &[start, end, color] : sys.debugLines) {
			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
			const float x1 = start.x - viewport.x;
			const float y1 = start.y - viewport.y;
			const float x2 = end.x - viewport.x;
			const float y2 = end.y - viewport.y;
			SDL_RenderLine(renderer, x1, y1, x2, y2);
		}
	}
}
void DebugSystem::updateSettings(SDL_Renderer *renderer)
{
	PROFILE_SCOPE("Debug::Update Settings");

	static bool lastVSync;
	if (lastVSync != instance().getRenderOptions().vSync) {
		SDL_SetRenderVSync(renderer, instance().getRenderOptions().vSync);
		lastVSync = instance().getRenderOptions().vSync;
	}
}

void DebugSystem::drawUI()
{
	PROFILE_SCOPE("Debug::Construct imGui");

	DebugSystem &sys = instance();

	if (!sys.enabled)
		return;

	sys.updateFPS();

	if (ImGui::Begin("Debug Control Panel")) {

		ImGui::Text("Performance");
		ImGui::SameLine();
		ImGui::TextDisabled("(%.1f FPS)", ImGui::GetIO().Framerate);
		ImGui::SameLine();
		ImGui::TextDisabled("(%.1f Average)",
		                    std::accumulate(sys.fpsHistory.begin(), sys.fpsHistory.end(), 0.0f) / sys.fpsHistorySize);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show Collision Boxes", &sys.render.showBoundingBoxes);
			ImGui::Checkbox("Show Hit Boxes", &sys.render.showHitBoxes);
			ImGui::Checkbox("Show Tooltip Boxes", &sys.render.showTooltipBoxes);
			ImGui::Checkbox("Show Raycasts", &sys.render.showRaycasts);
			ImGui::Checkbox("VSync", &sys.render.vSync);
		}

		if (ImGui::CollapsingHeader("Player Debug Info", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show State", &sys.player.showState);
			ImGui::Checkbox("Show Position", &sys.player.showPosition);
			ImGui::Checkbox("Show Health", &sys.player.showHP);
		}
		if (ImGui::CollapsingHeader("Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("God Mode", &sys.cheats.godMode);
			ImGui::Checkbox("No Clip", &sys.cheats.noClip);
			ImGui::Checkbox("Unlimited Jumps", &sys.cheats.unlimitedJumps);
		}
		// Replaced hardcoded buttons with dynamic area selection
		if (ImGui::CollapsingHeader("Teleport", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (!sys.hasSpawnPoint) {
				ImGui::TextDisabled("No SpawnPoint found in loaded maps");
			}
			ImGui::Separator();
			if (!sys.maps.empty()) {
				static int selectedMapIdx = 0;
				if (selectedMapIdx >= static_cast<int>(sys.maps.size())) {
					selectedMapIdx = 0;
				}

				const char *previewValue = sys.maps[selectedMapIdx].name.c_str();
				if (ImGui::BeginCombo("Select Area", previewValue)) {
					for (int n = 0; n < static_cast<int>(sys.maps.size()); n++) {
						const bool isSelected = (selectedMapIdx == n);
						if (ImGui::Selectable(sys.maps[n].name.c_str(), isSelected))
							selectedMapIdx = n;
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::Button("Teleport to Selected Area")) {
					if (sys.registry) {
						const auto &map = sys.maps[selectedMapIdx];
						glm::vec2 target = {map.area.x, map.area.y}; // Default to map start
						bool foundSpawn = false;

						for (const auto &sp : sys.spawnPoints) {
							if (sp.x >= map.area.x && sp.x <= map.area.x + map.area.w && sp.y >= map.area.y
							    && sp.y <= map.area.y + map.area.h) {
								target = sp;
								foundSpawn = true;
								break;
							}
						}

						auto playerView = sys.registry->view<PlayerTag, PositionComponent>();
						if (playerView.size() == 1U) {
							const auto playerEntity = playerView.at(0U);
							auto &playerPos = playerView.get<PositionComponent>(playerEntity);
							playerPos.position = target;
							playerPos.previousPosition = target;

							if (auto velocity = sys.registry->tryGet<VelocityComponent>(playerEntity)) {
								velocity->get().velocity = {0.0f, 0.0f};
							}
						}
						if (foundSpawn) {
							DebugSystem::debug("Teleported to spawn point in " + map.name);
						} else {
							DebugSystem::debug("No spawn point found in " + map.name + ", teleported to origin.");
						}
					}
				}
			} else {
				ImGui::TextDisabled("No areas loaded from Tiled.");
			}
		}
		if (sys.player.showPosition) {
			ImGui::SeparatorText("PlayerPosition");
			auto playerView = sys.registry->view<PlayerTag, PositionComponent>();
			const auto &pos = playerView.get<PositionComponent>(playerView.at(0)).position;
			ImGui::Text("X: %.2f Y: %.2f", pos.x, pos.y);
			// ImGui::Text("MaxEntity: %d", sys.registry->getMaxEntity());
		}
		if (sys.player.showHP) {
			ImGui::SeparatorText("HP");
			auto playerView = sys.registry->view<PlayerTag, HealthComponent>();
			const auto &health = playerView.get<HealthComponent>(playerView.at(0));
			ImGui::Text("%d/%d", health.health, health.maxHealth);
		}
		if (ImGui::CollapsingHeader("System Profiler")) {
			auto &[results] = Profiler::instance();

			if (results.empty()) {
				ImGui::TextDisabled("No systems profiled yet.");
			}
			int id = 0;
			for (auto &[name, data] : results) {
				const float average =
				    std::accumulate(data.history.begin(), data.history.end(), 0.0f) / data.history.size();

				ImVec4 color =
				    (average < 1.0f) ? ImVec4(0, 1, 0, 1) : (average < 5.0f ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1));

				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Text("%s: %f ns", std::string(name).c_str(), average * 1000);
				ImGui::PopStyleColor();
				// Draw the graph
				ImGui::PlotLines(std::string(name).c_str(), data.history.data(), data.history.size(), data.offset,
				                 nullptr, 0.0f,
				                 data.maxTime.count() * 1.1f, // <--- Passed as raw float here
				                 ImVec2(0, 40));
				ImGui::Spacing();
			}
		}

		if (!sys.debugMessages.empty()) {
			ImGui::SeparatorText("Frame Logs");

			if (ImGui::BeginChild("LogRegion", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar)) {
				for (const auto &msg : sys.debugMessages) {
					ImGui::TextUnformatted(msg.c_str());
				}
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}
void DebugSystem::drawCollisionBoxes() const
{
	if (!instance().getRenderOptions().showBoundingBoxes || !registry)
		return;

	for (auto view = registry->view<CollisionComponent, PositionComponent>(); const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[collision] = view.get<CollisionComponent>(entity);

		const auto transform = registry->tryGet<TransformComponent>(entity);
		const auto scale = transform ? &transform->get().scale : nullptr;
		SDL_FRect col = GameEngine::Utils::HitBoxUtils::getHitBox(position, collision, scale);

		const SDL_Color color =
		    registry->has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
		addRect(col, color);
	}
}

void DebugSystem::drawProjectileHitBox() const
{
	for (auto view = registry->view<ProjectileComponent, HitBoxComponent, PositionComponent>();
	     const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &hitBox = view.get<HitBoxComponent>(entity).bodyHitBox;
		const auto transform = registry->tryGet<TransformComponent>(entity);
		const auto scale = transform ? &transform->get().scale : nullptr;
		SDL_FRect hurtBoxRect = GameEngine::Utils::HitBoxUtils::getHitBox(position, hitBox, scale);
		constexpr SDL_Color projectileHurtBoxColor = {0, 255, 255, 255};
		addRect(hurtBoxRect, projectileHurtBoxColor);
	}
}

void DebugSystem::drawPatrolBoxes(const std::vector<unsigned>::value_type entity, SDL_FRect hurtBoxRect) const
{
	const auto ranged = registry->tryGet<RangedAttackComponent>(entity);
	const auto chase = registry->tryGet<ChaseComponent>(entity);

	if (const auto patrol = registry->tryGet<PatrolComponent>(entity); !ranged && chase && patrol) {
		SDL_FRect rangeRect =
		    GameEngine::Utils::DetectionRangeUtils::getMeleeDetectionRect(patrol->get(), chase->get());

		constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
		addRect(rangeRect, rangeColor);
	}

	if (ranged && chase) {
		const glm::vec2 center = GameEngine::Utils::DetectionRangeUtils::getRectCenter(hurtBoxRect);

		SDL_FRect rangeRect = GameEngine::Utils::DetectionRangeUtils::getRangedDetectionRect(chase->get(), center);

		constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
		addRect(rangeRect, rangeColor);
	}
}
void DebugSystem::drawHitBoxes() const
{
	if (!instance().getRenderOptions().showHitBoxes || registry == nullptr) {
		return;
	}

	drawProjectileHitBox();

	for (auto view = registry->view<HitBoxComponent, HurtBoxComponent, RenderComponent, PositionComponent>();
	     const auto entity : view) {

		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &hitbox = view.get<HitBoxComponent>(entity);
		const auto &hurtbox = view.get<HurtBoxComponent>(entity);
		const auto &render = view.get<RenderComponent>(entity);
		const bool facingLeft = render.isFacingLeft();
		SDL_FRect referenceBox = hurtbox.hurtBox;
		if (const auto sprite = registry->tryGet<SpriteComponent>(entity)) {
			const auto &tileSize = sprite->get().tileSize;
			referenceBox = {0.0f, 0.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
		} else if (const auto collision = registry->tryGet<CollisionComponent>(entity)) {
			referenceBox = collision->get().collisionBox;
		}
		const auto transform = registry->tryGet<TransformComponent>(entity);
		const auto scale = transform ? &transform->get().scale : nullptr;
		SDL_FRect localHurtBox = hurtbox.hurtBox;
		if (facingLeft) {
			localHurtBox.x = referenceBox.x + referenceBox.w - (hurtbox.hurtBox.x + hurtbox.hurtBox.w);
		}
		const auto hitBoxRect =
		    GameEngine::Utils::HitBoxUtils::getWeaponHitBoxRect(position, hitbox, localHurtBox, scale, facingLeft);

		constexpr SDL_Color hitboxColor = {0, 255, 0, 255};
		addRect(hitBoxRect, hitboxColor);

		const SDL_FRect bodyHitBoxRect = GameEngine::Utils::HitBoxUtils::getHitBoxFacing(
		    position, hitbox.bodyHitBox, referenceBox, scale, facingLeft);
		const SDL_FRect hurtBoxRect = GameEngine::Utils::HitBoxUtils::getHitBoxFacing(position, hurtbox.hurtBox,
		                                                                              referenceBox, scale, facingLeft);
		constexpr SDL_Color bodyHitBoxColor = {255, 0, 0, 255};
		addRect(bodyHitBoxRect, bodyHitBoxColor);

		constexpr SDL_Color hurtBoxColor = {255, 0, 255, 255};
		addRect(hurtBoxRect, hurtBoxColor);

		drawPatrolBoxes(entity, hurtBoxRect);
	}
}

void DebugSystem::drawTooltipBoxes() const
{
	if (!instance().getRenderOptions().showTooltipBoxes || registry == nullptr) {
		return;
	}
	auto view = registry->view<TooltipComponent>();
	for (const auto entity : view) {
		const auto &[text, box] = view.get<TooltipComponent>(entity);
		constexpr SDL_Color tooltipBoxColor = {255, 255, 0, 255};
		addRect(box, tooltipBoxColor);
	}
}

void DebugSystem::updateFPS()
{
	fpsHistory.at(fpsOffset) = ImGui::GetIO().Framerate;
	fpsOffset = (fpsOffset + 1) % fpsHistorySize;
}
