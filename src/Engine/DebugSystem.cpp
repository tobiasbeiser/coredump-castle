#include "DebugSystem.hpp"

#include "ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
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

	// Render debug rectangles
	for (const auto &[rect, color] : sys.debugRects) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		SDL_FRect drawRect = rect;
		drawRect.x -= viewport.x;
		drawRect.y -= viewport.y;

		SDL_RenderRect(renderer, &drawRect);
	}

	// Render debug lines (raycasts)
	for (const auto &[start, end, color] : sys.debugLines) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		const float startX = start.x - viewport.x;
		const float startY = start.y - viewport.y;
		const float endX = end.x - viewport.x;
		const float endY = end.y - viewport.y;

		SDL_RenderLine(renderer, startX, startY, endX, endY);
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