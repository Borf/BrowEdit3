#define IMGUI_DEFINE_MATH_OPERATORS
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/MapView.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/Action.h>
#include <browedit/gl/Texture.h>
#include <browedit/Icons.h>
#include <imgui_internal.h>

void BrowEdit::showTextureBrushWindow()
{
	if (!activeMapView)
		return;

	auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
	auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();
	if (ImGui::Begin("Texture Brush Options"))
	{
		if (!activeMapView)
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginCombo("Texture", util::iso_8859_1_to_utf8(gnd->textures[activeMapView->textureSelected]->file).c_str()))
		{
			for (auto i = 0; i < gnd->textures.size(); i++)
			{
				if (ImGui::Selectable(util::iso_8859_1_to_utf8("##" + gnd->textures[i]->file).c_str(), i == activeMapView->textureSelected, 0, ImVec2(0, 64)))
				{
					activeMapView->textureSelected = i;
				}
				ImGui::SameLine(0);
				ImGui::Image((ImTextureID)(long long)gndRenderer->textures[i]->id, ImVec2(64, 64));
				ImGui::SameLine(80);
				ImGui::Text(util::iso_8859_1_to_utf8(gnd->textures[i]->file).c_str());

			}
			ImGui::EndCombo();
		}

		static bool snapUv = true;
		static int snapDivX = 4;
		static int snapDivY = 4;

			
		ImGui::DragInt("Snap X Divider", &snapDivX, 1, 1, 20);
		ImGui::DragInt("Snap Y Divider", &snapDivY, 1, 1, 20);

		toolBarToggleButton("SnapUV", snapUv ? ICON_GRID_SNAP_ON : ICON_GRID_SNAP_OFF, &snapUv, "Snap UV editor");

		bool changed = false;
		{ //UV editor
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			const ImGuiStyle& style = ImGui::GetStyle();
			const ImGuiIO& IO = ImGui::GetIO();
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const float avail = ImGui::GetContentRegionAvailWidth();
			const float dim = ImMin(avail, 300.0f);
			ImVec2 Canvas(dim, dim);

			ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
			ImGui::ItemSize(bb);
			ImGui::ItemAdd(bb, NULL);
			const ImGuiID id = window->GetID("Texture Edit");
			//hovered |= 0 != ImGui::IsItemHovered(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id);
			ImGuiContext& g = *GImGui;

			ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
			window->DrawList->AddImage((ImTextureID)(long long)gndRenderer->textures[activeMapView->textureSelected]->id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImVec2(0, 1), ImVec2(1, 0));


			if(snapDivX > 0)
				for (float x = 0; x < 1; x += (1.0f / snapDivX))
					window->DrawList->AddLine(
						ImVec2(bb.Min.x + x * bb.GetWidth(), bb.Max.y - 0 * bb.GetHeight()),
						ImVec2(bb.Min.x + x * bb.GetWidth(), bb.Max.y - 1 * bb.GetHeight()), ImGui::GetColorU32(ImGuiCol_Text, 0.25), 1.0f);
			if(snapDivY > 0)
				for (float y = 0; y < 1; y += (1.0f / snapDivY))
					window->DrawList->AddLine(
						ImVec2(bb.Min.x + 0 * bb.GetWidth(), bb.Max.y - y * bb.GetHeight()),
						ImVec2(bb.Min.x + 1 * bb.GetWidth(), bb.Max.y - y * bb.GetHeight()), ImGui::GetColorU32(ImGuiCol_Text, 0.25), 1.0f);

			window->DrawList->AddRectFilled(
				ImVec2(bb.Min.x + activeMapView->textureEditUv1.x * bb.GetWidth(), bb.Max.y - activeMapView->textureEditUv1.y * bb.GetHeight()),
				ImVec2(bb.Min.x + activeMapView->textureEditUv2.x * bb.GetWidth(), bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight()),
				ImGui::GetColorU32(ImVec4(1, 0, 0, 0.2f)));

			window->DrawList->AddRect(
				ImVec2(bb.Min.x + activeMapView->textureEditUv1.x * bb.GetWidth(), bb.Max.y - activeMapView->textureEditUv1.y * bb.GetHeight()),
				ImVec2(bb.Min.x + activeMapView->textureEditUv2.x * bb.GetWidth(), bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight()),
				ImGui::GetColorU32(ImGuiCol_Text, 1.0f), 0, 0, 2.0f);

			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			int i = 0;
			static bool c = false;
			static bool dragged = false;

			glm::vec2 offsets[] = { glm::vec2(0,0), glm::vec2(0,1), glm::vec2(1,1), glm::vec2(1,0) };

			for (int i = 0; i < 4; i++)
			{
				ImVec2 pos = ImVec2(bb.Min.x + glm::mix(activeMapView->textureEditUv1.x, activeMapView->textureEditUv2.x, offsets[i].x)* bb.GetWidth(), 
									bb.Max.y - glm::mix(activeMapView->textureEditUv1.y, activeMapView->textureEditUv2.y, offsets[i].y) * bb.GetHeight());
				window->DrawList->AddCircle(pos, 5, ImGui::GetColorU32(ImGuiCol_Text, 1), 0, 2.0f);
				ImGui::PushID(i);
				ImGui::SetCursorScreenPos(pos - ImVec2(10, 10));
				ImGui::InvisibleButton("button", ImVec2(2 * 10, 2 * 10));
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip("(%4.3f, %4.3f)", glm::mix(activeMapView->textureEditUv1.x, activeMapView->textureEditUv2.x, offsets[i].x), glm::mix(activeMapView->textureEditUv1.y, activeMapView->textureEditUv2.y, offsets[i].y));
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
				{
					float& x = offsets[i].x == 0 ? activeMapView->textureEditUv1.x : activeMapView->textureEditUv2.x;
					float& y = offsets[i].y == 0 ? activeMapView->textureEditUv1.y : activeMapView->textureEditUv2.y;

					x = (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x;
					y = 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y;

					bool snap = snapUv;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;

					if (snap && snapDivX > 0 && snapDivY > 0)
					{
						glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
						x = glm::round(x / inc.x) * inc.x;
						y = glm::round(y / inc.y) * inc.y;
					}
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
				ImGui::PopID();
			}
			ImGui::SetCursorScreenPos(cursorPos);
		}
		ImGui::DragInt("Brush Width", &activeMapView->textureBrushWidth, 1, 1, 20);
		glm::vec2 uvSize = activeMapView->textureEditUv2 - activeMapView->textureEditUv1;
		float textureBrushHeight = activeMapView->textureBrushWidth * (uvSize.y / uvSize.x);
		if (glm::abs(textureBrushHeight - glm::floor(textureBrushHeight)) > 0.01)
		ImGui::TextColored(ImVec4(1,0,0,1), "ERROR: THIS WIDTH WON'T WORK, HEIGHT WOULD BE %f", textureBrushHeight);


		ImGui::End();
	}
}
