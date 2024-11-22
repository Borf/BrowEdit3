#define IMGUI_DEFINE_MATH_OPERATORS
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/Action.h>
#include <browedit/gl/Texture.h>
#include <browedit/Icons.h>
#include <browedit/HotkeyRegistry.h>
#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void BrowEdit::showTextureBrushWindow()
{
	if (!activeMapView)
		return;

	auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
	auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();

	if (gnd->textures.size() == 0)
		return;

	if (!ImGui::GetIO().WantTextInput && activeMapView)
	{
		if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
		{
			activeMapView->textureBrushWidth++;
			activeMapView->textureBrushHeight++;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
		{
			if (activeMapView->textureBrushWidth > 1)
				activeMapView->textureBrushWidth--;
			if (activeMapView->textureBrushHeight > 1)
				activeMapView->textureBrushHeight--;
		}
	}

	if (ImGui::Begin("Texture Brush Options"))
	{
		if (!activeMapView)
		{
			ImGui::End();
			return;
		}



		if (toolBarToggleButton("stamp", ICON_TEXTUREBRUSH_STAMP, textureBrushMode == TextureBrushMode::Stamp, "Texture Stamping"))
			textureBrushMode = TextureBrushMode::Stamp;
		ImGui::SameLine();
		if (toolBarToggleButton("bucket", ICON_TEXTUREBRUSH_BUCKET, textureBrushMode == TextureBrushMode::Fill, "Texture filling"))
			textureBrushMode = TextureBrushMode::Fill;
		ImGui::SameLine();
		if (toolBarToggleButton("select", ICON_TEXTUREBRUSH_SELECT, textureBrushMode == TextureBrushMode::Select, "Texture Selecting & fill"))
			textureBrushMode = TextureBrushMode::Select;
		ImGui::SameLine();
		if (toolBarToggleButton("dropper", ICON_DROPPER, textureBrushMode == TextureBrushMode::Dropper, "Picking texture with dropper"))
		{
			brushModeBeforeDropper = textureBrushMode;
			textureBrushMode = TextureBrushMode::Dropper;
		}


		if (textureBrushMode == TextureBrushMode::Select)
		{ //todo: put this somewhere shared between height & texture mode
			if (toolBarToggleButton("Rectangle", ICON_SELECT_RECTANGLE, selectTool == SelectTool::Rectangle, "Rectangle Select", ImVec4(1, 1, 1, 1)))
				selectTool = SelectTool::Rectangle;
			ImGui::SameLine();
			if (toolBarToggleButton("Lasso", ICON_SELECT_LASSO, selectTool == SelectTool::Lasso, "Rectangle Select", ImVec4(1, 1, 1, 1)))
				selectTool = SelectTool::Lasso;
			ImGui::SameLine();
			if (toolBarToggleButton("WandTex", ICON_SELECT_WAND_TEX, selectTool == SelectTool::WandTex, "Magic Wand (Texture)", ImVec4(1, 1, 1, 1)))
				selectTool = SelectTool::WandTex;
			ImGui::SameLine();
			if (toolBarToggleButton("WandHeight", ICON_SELECT_WAND_HEIGHT, selectTool == SelectTool::WandHeight, "Magic Wand (Height)", ImVec4(1, 1, 1, 1)))
				selectTool = SelectTool::WandHeight;
			ImGui::SameLine();
			if (toolBarToggleButton("AllTex", ICON_SELECT_ALL_TEX, selectTool == BrowEdit::SelectTool::AllTex, "Select All (Texture)", ImVec4(1, 1, 1, 1)))
				selectTool = BrowEdit::SelectTool::AllTex;
			ImGui::SameLine();
			if (toolBarToggleButton("AllHeight", ICON_SELECT_ALL_HEIGHT, selectTool == BrowEdit::SelectTool::AllHeight, "Select All (Height)", ImVec4(1, 1, 1, 1)))
				selectTool = BrowEdit::SelectTool::AllHeight;
		}

		if (ImGui::BeginCombo("Texture", util::iso_8859_1_to_utf8(gnd->textures[activeMapView->textureSelected]->file).c_str(), ImGuiComboFlags_HeightLargest))
		{
			for (auto i = 0; i < gnd->textures.size(); i++)
			{
				if (ImGui::Selectable(util::iso_8859_1_to_utf8("##" + gnd->textures[i]->file).c_str(), i == activeMapView->textureSelected, 0, ImVec2(0, 64)))
				{
					activeMapView->textureSelected = i;
				}
				ImGui::SameLine(0);
				ImGui::Image((ImTextureID)(long long)gndRenderer->textures[i]->id(), ImVec2(64, 64));
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
		ImGui::SameLine();
		if (toolBarButton("RotateL", ICON_ROTATE, "Rotate texture left", ImVec4(1,1,1,1)) || (ImGui::IsKeyPressed('L') && !ImGui::GetIO().WantTextInput))
		{ //00 -> 10 -> 11 -> 01
			activeMapView->textureBrushFlipD = !activeMapView->textureBrushFlipD;
			
			if (activeMapView->textureBrushFlipWeird())
			{
				bool tmp = activeMapView->textureBrushFlipV;
				activeMapView->textureBrushFlipV = !activeMapView->textureBrushFlipH;
				activeMapView->textureBrushFlipH = tmp;
			}
			else
			{
				bool tmp = activeMapView->textureBrushFlipH;
				activeMapView->textureBrushFlipH = !activeMapView->textureBrushFlipV;
				activeMapView->textureBrushFlipV = tmp;
			}
			glm::vec2 uv1 = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.5f, 0)), glm::radians(90.0f), glm::vec3(0, 0, 1)), glm::vec3(-0.5f, -0.5f, 0.0f)) * glm::vec4(activeMapView->textureEditUv1, 0, 1);
			glm::vec2 uv2 = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.5f, 0)), glm::radians(90.0f), glm::vec3(0, 0, 1)), glm::vec3(-0.5f, -0.5f, 0.0f)) * glm::vec4(activeMapView->textureEditUv2, 0, 1);
			activeMapView->textureEditUv1 = glm::min(uv1, uv2);
			activeMapView->textureEditUv2 = glm::max(uv1, uv2);
			if (activeMapView->textureBrushAutoFlipSize)
				std::swap(activeMapView->textureBrushWidth, activeMapView->textureBrushHeight);
		}
		ImGui::SameLine();
		if (toolBarButton("RotateR", ICON_ROTATE_RIGHT, "Rotate texture right", ImVec4(1, 1, 1, 1)) || (ImGui::IsKeyPressed('R') && !ImGui::GetIO().WantTextInput))
		{ //00 -> 01 -> 11 -> 10
			activeMapView->textureBrushFlipD = !activeMapView->textureBrushFlipD;
			if (activeMapView->textureBrushFlipWeird())
			{
				bool tmp = activeMapView->textureBrushFlipH;
				activeMapView->textureBrushFlipH = !activeMapView->textureBrushFlipV;
				activeMapView->textureBrushFlipV = tmp;
			}
			else
			{
				bool tmp = activeMapView->textureBrushFlipV;
				activeMapView->textureBrushFlipV = !activeMapView->textureBrushFlipH;
				activeMapView->textureBrushFlipH = tmp;
			}

			glm::vec2 uv1 = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.5f, 0)), glm::radians(-90.0f), glm::vec3(0, 0, 1)), glm::vec3(-0.5f, -0.5f, 0.0f)) * glm::vec4(activeMapView->textureEditUv1, 0, 1);
			glm::vec2 uv2 = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0), glm::vec3(0.5f, 0.5f, 0)), glm::radians(-90.0f), glm::vec3(0, 0, 1)), glm::vec3(-0.5f, -0.5f, 0.0f)) * glm::vec4(activeMapView->textureEditUv2, 0, 1);
			activeMapView->textureEditUv1 = glm::min(uv1, uv2);
			activeMapView->textureEditUv2 = glm::max(uv1, uv2);
			if (activeMapView->textureBrushAutoFlipSize)
				std::swap(activeMapView->textureBrushWidth, activeMapView->textureBrushHeight);
		}
		ImGui::SameLine();
		if (toolBarButton("MirrorH", ICON_MIRROR_HORIZONTAL, "Mirror Horizontal", ImVec4(1, 1, 1, 1)) || (ImGui::IsKeyPressed('H') && !ImGui::GetIO().WantTextInput))
		{
			if(activeMapView->textureBrushFlipD)
				activeMapView->textureBrushFlipV = !activeMapView->textureBrushFlipV;
			else
				activeMapView->textureBrushFlipH = !activeMapView->textureBrushFlipH;

			activeMapView->textureEditUv1.x = 1 - activeMapView->textureEditUv1.x;
			activeMapView->textureEditUv2.x = 1 - activeMapView->textureEditUv2.x;
			std::swap(activeMapView->textureEditUv1.x, activeMapView->textureEditUv2.x);
		}
		ImGui::SameLine();
		if (toolBarButton("MirrorV", ICON_MIRROR_VERTICAL, "Mirror Vertical", ImVec4(1, 1, 1, 1)) || (ImGui::IsKeyPressed('V') && !ImGui::GetIO().WantTextInput))
		{
			if (!activeMapView->textureBrushFlipD)
				activeMapView->textureBrushFlipV = !activeMapView->textureBrushFlipV;
			else
				activeMapView->textureBrushFlipH = !activeMapView->textureBrushFlipH;

			activeMapView->textureEditUv1.y = 1 - activeMapView->textureEditUv1.y;
			activeMapView->textureEditUv2.y = 1 - activeMapView->textureEditUv2.y;
			std::swap(activeMapView->textureEditUv1.y, activeMapView->textureEditUv2.y);
		}
		ImGui::Text("%d %d %d", activeMapView->textureBrushFlipD ? 1 : 0, activeMapView->textureBrushFlipH ? 1 : 0, activeMapView->textureBrushFlipV ? 1 : 0);


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
			ImGuiContext& g = *GImGui;

			ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);

			ImVec2 uv1(0, 0);
			ImVec2 uv4(1, 1);
			ImVec2 uv2(uv4.x, uv1.y);
			ImVec2 uv3(uv1.x, uv4.y);
			if (activeMapView->textureBrushFlipD)
				std::swap(uv2, uv3);

			if (activeMapView->textureBrushFlipH)
			{
				uv1.x = 1 - uv1.x;
				uv2.x = 1 - uv2.x;
				uv3.x = 1 - uv3.x;
				uv4.x = 1 - uv4.x;
			}
			if (activeMapView->textureBrushFlipV)
			{
				uv1.y = 1 - uv1.y;
				uv2.y = 1 - uv2.y;
				uv3.y = 1 - uv3.y;
				uv4.y = 1 - uv4.y;
			}

			window->DrawList->AddImageQuad((ImTextureID)(long long)gndRenderer->textures[activeMapView->textureSelected]->id(),
				ImVec2(bb.Min.x + 1, bb.Min.y + 1), ImVec2(bb.Max.x - 1, bb.Min.y + 1), ImVec2(bb.Max.x - 1, bb.Max.y - 1), ImVec2(bb.Min.x + 1, bb.Max.y - 1),
				uv1, uv2, uv4, uv3);


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

			for (int i = 0; i < 4; i++) //4 corners
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
					x = glm::clamp(x, 0.0f, 1.0f);
					y = glm::clamp(y, 0.0f, 1.0f);
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
				ImGui::PopID();
			}
			//left
			{
				ImVec2 pos = ImVec2(bb.Min.x + activeMapView->textureEditUv1.x * bb.GetWidth(),
					bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight());
				ImGui::SetCursorScreenPos(pos - ImVec2(10, 10));
				ImGui::InvisibleButton("left", ImVec2(2 * 10, 2 * 10 + (activeMapView->textureEditUv2.y - activeMapView->textureEditUv1.y) * bb.GetHeight()));
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip("(%4.3f)", activeMapView->textureEditUv1.x);
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
				{
					activeMapView->textureEditUv1.x = (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x;
					bool snap = snapUv;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;

					if (snap && snapDivX > 0 && snapDivY > 0)
					{
						glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
						activeMapView->textureEditUv1.x = glm::round(activeMapView->textureEditUv1.x / inc.x) * inc.x;
					}
					activeMapView->textureEditUv1.x = glm::clamp(activeMapView->textureEditUv1.x, 0.0f, 1.0f);
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
			}
			//right
			{
				ImVec2 pos = ImVec2(bb.Min.x + activeMapView->textureEditUv2.x * bb.GetWidth(),
									bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight());
				ImGui::SetCursorScreenPos(pos - ImVec2(10, 10));
				ImGui::InvisibleButton("right", ImVec2(2 * 10, 2 * 10 + (activeMapView->textureEditUv2.y - activeMapView->textureEditUv1.y) * bb.GetHeight()));
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip("(%4.3f)", activeMapView->textureEditUv2.x);
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
				{
					activeMapView->textureEditUv2.x = (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x;
					bool snap = snapUv;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;

					if (snap && snapDivX > 0 && snapDivY > 0)
					{
						glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
						activeMapView->textureEditUv2.x = glm::round(activeMapView->textureEditUv2.x / inc.x) * inc.x;
					}
					activeMapView->textureEditUv2.x = glm::clamp(activeMapView->textureEditUv2.x, 0.0f, 1.0f);
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
			}
			//top
			{
				ImVec2 pos = ImVec2(bb.Min.x + activeMapView->textureEditUv1.x * bb.GetWidth(),
									bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight());
				ImGui::SetCursorScreenPos(pos - ImVec2(10, 10));
				ImGui::InvisibleButton("top", ImVec2(2 * 10 + (activeMapView->textureEditUv2.x - activeMapView->textureEditUv1.x) * bb.GetWidth(), 2 * 10));
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip("(%4.3f)", activeMapView->textureEditUv2.y);
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
				{
					activeMapView->textureEditUv2.y = 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y;
					bool snap = snapUv;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;

					if (snap && snapDivX > 0 && snapDivY > 0)
					{
						glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
						activeMapView->textureEditUv2.y = glm::round(activeMapView->textureEditUv2.y / inc.y) * inc.y;
					}
					activeMapView->textureEditUv2.y = glm::clamp(activeMapView->textureEditUv2.y, 0.0f, 1.0f);
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
			}
			//bottom
			{
				ImVec2 pos = ImVec2(bb.Min.x + activeMapView->textureEditUv1.x * bb.GetWidth(),
					bb.Max.y - activeMapView->textureEditUv2.y * bb.GetHeight());
				ImGui::SetCursorScreenPos(pos - ImVec2(10, 10 - (activeMapView->textureEditUv2.y - activeMapView->textureEditUv1.y) * bb.GetHeight()));
				ImGui::InvisibleButton("bottom", ImVec2(2 * 10 + (activeMapView->textureEditUv2.x - activeMapView->textureEditUv1.x) * bb.GetWidth(), 2 * 10));
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip("(%4.3f)", activeMapView->textureEditUv1.y);
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
				{
					activeMapView->textureEditUv1.y = 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y;
					bool snap = snapUv;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;
					if (snap && snapDivX > 0 && snapDivY > 0)
					{
						glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
						activeMapView->textureEditUv1.y = glm::round(activeMapView->textureEditUv1.y / inc.y) * inc.y;
					}
					activeMapView->textureEditUv1.y = glm::clamp(activeMapView->textureEditUv1.y, 0.0f, 1.0f);
					dragged = true;
				}
				else if (ImGui::IsMouseReleased(0) && dragged)
				{
					dragged = false;
					changed = true;
				}
			}

			//dragging into void
			ImGui::SetCursorScreenPos(bb.Min);
			ImGui::InvisibleButton("button", bb.GetSize());
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
			{
				static glm::vec2 dragStart;
				if (!dragged)
				{
					dragStart.x = (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x;
					dragStart.y = 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y;
				}
				dragged = true;
				activeMapView->textureEditUv1.x = glm::min(dragStart.x, (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x);
				activeMapView->textureEditUv1.y = glm::min(dragStart.y, 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y);

				activeMapView->textureEditUv2.x = glm::max(dragStart.x, (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x);
				activeMapView->textureEditUv2.y = glm::max(dragStart.y, 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y);

				bool snap = snapUv;
				if (ImGui::GetIO().KeyShift)
					snap = !snap;
				if (snap && snapDivX > 0 && snapDivY > 0)
				{
					glm::vec2 inc(1.0f / snapDivX, 1.0f / snapDivY);
					activeMapView->textureEditUv1 = glm::round(activeMapView->textureEditUv1 / inc) * inc;
					activeMapView->textureEditUv2 = glm::round(activeMapView->textureEditUv2 / inc) * inc;
				}
			}
			if (ImGui::IsItemActive() && ImGui::IsMouseReleased(0) && dragged)
				dragged = false;

			ImGui::SetCursorScreenPos(cursorPos);
		}
		


		if (ImGui::InputInt("Brush Width", &activeMapView->textureBrushWidth, 1))
			if (activeMapView->textureBrushWidth < 1)
				activeMapView->textureBrushWidth = 1;
		if(ImGui::InputInt("Brush Height", &activeMapView->textureBrushHeight, 1))
			if (activeMapView->textureBrushHeight < 1)
				activeMapView->textureBrushHeight = 1;

		glm::vec2 uvSize = activeMapView->textureEditUv2 - activeMapView->textureEditUv1;
		// Updates brush size based on selection
		if (activeMapView->textureBrushUpdateBySelection) {
			activeMapView->textureBrushWidth = (int)(uvSize.x * 4);
			activeMapView->textureBrushHeight = (int)(uvSize.y * 4);
		}
				
		float textureBrushHeight = activeMapView->textureBrushWidth * (uvSize.y / uvSize.x);
		if(glm::abs(activeMapView->textureBrushHeight - textureBrushHeight) < 0.01)
			ImGui::Text("For proper aspect ratio, height should be %f", textureBrushHeight);
		else
			ImGui::TextColored(ImVec4(1,0,0,1), "For proper aspect ratio, height should be %f", textureBrushHeight);
		
		glm::vec2 uv1 = activeMapView->textureEditUv1;
		glm::vec2 uv2 = activeMapView->textureEditUv2;
		activeMapView->textureEditUv1 = glm::min(uv1, uv2);
		activeMapView->textureEditUv2 = glm::max(uv1, uv2);


		hotkeyButton("Swap brush size", HotkeyAction::TextureEdit_SwapBrushSize);

		ImGui::Checkbox("Automatically flip brush size", &activeMapView->textureBrushAutoFlipSize);
		ImGui::Checkbox("Keep Shadows", &activeMapView->textureBrushKeepShadow);
		ImGui::Checkbox("Keep Colors", &activeMapView->textureBrushKeepColor);
		ImGui::Checkbox("Lock brush size by selection", &activeMapView->textureBrushUpdateBySelection);
		if (textureBrushMode == TextureBrushMode::Select)
		{
			ImGui::InputInt2("Brush Offset", glm::value_ptr(textureFillOffset), 1);
		}


		ImGui::End();
	}
}
