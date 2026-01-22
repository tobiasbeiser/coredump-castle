#include "DebugSystem.hpp"

#include "ECS/Components/Core/PositionComponent.hpp"
#include "ECS/Components/Core/TransformComponent.hpp"
#include "ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "ECS/Components/Gameplay/MovementComponent.hpp"
#include "ECS/Components/Physics/CollisionComponent.hpp"
#include "ECS/Components/Render/RenderComponent.hpp"
#include "ECS/Components/Tags/PlayerTag.hpp"
#include "ECS/View.hpp"

#include <imgui.h>

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
	DebugSystem &sys = instance();
	if (!sys.render.showRaycasts)
		return;
	sys.debugLines.push_back({start, end, color});
}

void DebugSystem::clear()
{
	instance().debugRects.clear();
	instance().debugLines.clear();
	instance().debugMessages.clear();
}

void DebugSystem::renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport)
{
	DebugSystem &sys = instance();
	sys.drawCollisionBoxes();
	sys.drawHitBoxes();

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

			const float startX = start.x - viewport.x;
			const float startY = start.y - viewport.y;
			const float endX = end.x - viewport.x;
			const float endY = end.y - viewport.y;

			SDL_RenderLine(renderer, startX, startY, endX, endY);
		}
	}
}

void DebugSystem::drawCollisionBoxes() const
{
	if (!instance().getRenderOptions().showBoundingBoxes || !registry)
		return;

	for (auto view = registry->view<CollisionComponent, PositionComponent>(); const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[collisionBox] = view.get<CollisionComponent>(entity);

		SDL_FRect col = collisionBox;
		col.x = position.x + col.x;
		col.y = position.y + col.y;

		const SDL_Color color =
		    registry->has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
		addRect(col, color);
	}
}

void DebugSystem::drawHitBoxes() const
{
	if (!instance().getRenderOptions().showHitBoxes || !registry)
		return;

	for (auto view = registry->view<HitBoxComponent, HurtBoxComponent, RenderComponent, PositionComponent>();
	     const auto entity : view) {

		const auto &[bodyHitBox, weaponHitBox] = view.get<HitBoxComponent>(entity);
		const auto &[hurtBox] = view.get<HurtBoxComponent>(entity);
		const auto &[layer, flipX] = view.get<RenderComponent>(entity);
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);

		glm::vec2 scale = glm::vec2(1.0f, 1.0f);
		if (registry->has<TransformComponent>(entity)) {
			scale = registry->get<TransformComponent>(entity).scale;
		}

		const bool facingLeft = flipX;

		SDL_FRect hitBoxRect = {0, 0, weaponHitBox.w * scale.x, weaponHitBox.h * scale.y};
		if (facingLeft) {
			hitBoxRect.x = position.x + hurtBox.x * scale.x - weaponHitBox.w * scale.x;
		} else {
			hitBoxRect.x = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
		}
		hitBoxRect.y = position.y + hurtBox.y * scale.y;

		if (hitBoxRect.w > 0.0f && hitBoxRect.h > 0.0f) {
			constexpr SDL_Color hitboxColor = {0, 255, 0, 255};
			addRect(hitBoxRect, hitboxColor);
		}

		SDL_FRect hurtBoxRect = {position.x + hurtBox.x * scale.x, position.y + hurtBox.y * scale.y,
		                         hurtBox.w * scale.x, hurtBox.h * scale.y};
		constexpr SDL_Color hurtBoxColor = {255, 0, 255, 255};
		addRect(hurtBoxRect, hurtBoxColor);

		if (!registry->has<RangedAttackComponent>(entity) && registry->has<ChaseComponent>(entity)) {
			const auto &chase = registry->get<ChaseComponent>(entity);
			const auto &patrol = registry->get<PatrolComponent>(entity);

			SDL_FRect rangeRect;
			rangeRect.w = patrol.maxBounds.x - patrol.minBounds.x;
			rangeRect.h = chase.detectionHeightRange * 2.0f;
			rangeRect.x = patrol.minBounds.x;
			rangeRect.y = patrol.minBounds.y - chase.detectionHeightRange;

			constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
			addRect(rangeRect, rangeColor);
		}

		if (registry->has<RangedAttackComponent>(entity)) {
			const auto &chase = registry->get<ChaseComponent>(entity);

			const float centerX = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x * 0.5f;
			const float centerY = position.y + hurtBox.y * scale.y + hurtBox.h * scale.y * 0.5f;

			SDL_FRect rangeRect;
			const auto detectionWidth = chase.detectionWidthRange.value_or(0.0f);
			rangeRect.w = detectionWidth * 2.0f;
			rangeRect.h = chase.detectionHeightRange * 2.0f;
			rangeRect.x = centerX - detectionWidth;
			rangeRect.y = centerY - chase.detectionHeightRange;

			constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
			addRect(rangeRect, rangeColor);
		}
	}
}

void DebugSystem::drawUI()
{
	DebugSystem &sys = instance();

	if (!sys.enabled)
		return;

	if (ImGui::Begin("Debug Control Panel")) {

		ImGui::Text("Performance");
		ImGui::SameLine();
		ImGui::TextDisabled("(%.1f FPS)", ImGui::GetIO().Framerate);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show Bounding Boxes", &sys.render.showBoundingBoxes);
			ImGui::Checkbox("Show Hit Boxes", &sys.render.showHitBoxes);
			ImGui::Checkbox("Show Raycasts", &sys.render.showRaycasts);
		}

		if (ImGui::CollapsingHeader("Player Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show State", &sys.player.showState);
			ImGui::Checkbox("God Mode", &sys.player.godMode);
			ImGui::Checkbox("No Clip", &sys.player.noClip);
		}

		if (sys.registry) {
			ImGui::Separator();
			if (ImGui::Button("Respawn")) {
				for (auto player = sys.registry->view<PlayerTag>(); const auto entity : player) {
					if (sys.registry->has<PendingRespawnComponent>(entity)) {
						continue;
					}
					sys.registry->registerComponent<PendingRespawnComponent>(entity, {});
				}
			}
			ImGui::Separator();
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