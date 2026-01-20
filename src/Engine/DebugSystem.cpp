#include "DebugSystem.hpp"
#include <imgui.h>

void DebugSystem::debug(const std::string &message)
{
	instance().debugMessages.push_back(message);
}

void DebugSystem::addRect(const SDL_FRect &rect, const SDL_Color &color)
{
	instance().debugRects.push_back({rect, color});
}

void DebugSystem::clear()
{
	instance().debugRects.clear();
	instance().debugMessages.clear();
}

void DebugSystem::renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport)
{
	for (DebugSystem &sys = instance(); const auto &[rect, color] : sys.debugRects) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		SDL_FRect drawRect = rect;
		drawRect.x -= viewport.x;
		drawRect.y -= viewport.y;

		SDL_RenderRect(renderer, &drawRect);
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
		}

		if (ImGui::CollapsingHeader("Player Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Show State", &sys.player.showState);
			ImGui::Checkbox("God Mode", &sys.player.godMode);
			ImGui::Checkbox("No Clip", &sys.player.noClip);
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