#include "DebugSystem.hpp"

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
				ImGui::Text("%s: %.3f ms", name.c_str(), average);
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
	}
	ImGui::End();
}