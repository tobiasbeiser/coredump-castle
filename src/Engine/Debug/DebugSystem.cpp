#include "DebugSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Profiler.hpp"

#include <imgui.h>
#include <numeric>

struct PatrolComponent;
struct ChaseComponent;
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
	sys.drawTooltopBoxes();

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

void DebugSystem::drawUI()
{
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
			ImGui::Checkbox("Show Bounding Boxes", &sys.render.showBoundingBoxes);
			ImGui::Checkbox("Show Hit Boxes", &sys.render.showHitBoxes);
			ImGui::Checkbox("Show Tooltip Boxes", &sys.render.showTooltipBoxes);
		}

		if (ImGui::CollapsingHeader("Player Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show State", &sys.player.showState);
			ImGui::Checkbox("Show Position", &sys.player.showPosition);
			ImGui::Checkbox("God Mode", &sys.player.godMode);
			ImGui::Checkbox("No Clip", &sys.player.noClip);
		}
		if (ImGui::CollapsingHeader("System Profiler", ImGuiTreeNodeFlags_DefaultOpen)) {
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
				ImGui::Text("%s: %f ns", name.c_str(), average * 1000);
				ImGui::PopStyleColor();
				// Draw the graph
				ImGui::PlotLines(name.c_str(), data.history.data(), data.history.size(), data.offset, nullptr, 0.0f,
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
		if (sys.player.showPosition) {
			ImGui::SeparatorText("PlayerPosition");
			const auto player = sys.registry->view<PlayerTag>().at(0);
			const auto &pos = sys.registry->get<PositionComponent>(player).position;
			ImGui::Text("X: %.2f Y: %.2f", pos.x, pos.y);
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

		SDL_FRect col = collision;
		col.x = position.x + col.x;
		col.y = position.y + col.y;

		const SDL_Color color =
		    registry->has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
		addRect(col, color);
	}
}
void DebugSystem::drawHitBoxes() const
{
	if (!instance().getRenderOptions().showHitBoxes || registry == nullptr) {
		return;
	}

	for (auto view =
	         registry
	             ->view<HitBoxComponent, HurtBoxComponent, RenderComponent, TransformComponent, PositionComponent>();
	     const auto entity : view) {

		if (const auto hitBoxRect = GameEngine::Utils::HitBoxUtils::getWeaponHitBoxRect(*registry, entity);
		    hitBoxRect.w > 0.0f && hitBoxRect.h > 0.0f) {
			constexpr SDL_Color hitboxColor = {0, 255, 0, 255};
			addRect(hitBoxRect, hitboxColor);
		}

		SDL_FRect hurtBoxRect = GameEngine::Utils::HitBoxUtils::getHurtBoxRect(*registry, entity);
		constexpr SDL_Color hurtBoxColor = {255, 0, 255, 255};
		addRect(hurtBoxRect, hurtBoxColor);

		if (!registry->has<RangedAttackComponent>(entity) && registry->has<ChaseComponent>(entity)) {
			const auto &chase = registry->get<ChaseComponent>(entity);
			const auto &patrol = registry->get<PatrolComponent>(entity);

			SDL_FRect rangeRect = GameEngine::Utils::DetectionRangeUtils::getMeleeDetectionRect(patrol, chase);

			constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
			addRect(rangeRect, rangeColor);
		}

		if (registry->has<RangedAttackComponent>(entity)) {
			const auto &chase = registry->get<ChaseComponent>(entity);
			const glm::vec2 center = GameEngine::Utils::DetectionRangeUtils::getRectCenter(hurtBoxRect);

			SDL_FRect rangeRect = GameEngine::Utils::DetectionRangeUtils::getRangedDetectionRect(chase, center);

			constexpr SDL_Color rangeColor = {255, 165, 0, 255}; // Orange
			addRect(rangeRect, rangeColor);
		}
	}
}

void DebugSystem::drawTooltopBoxes() const
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
