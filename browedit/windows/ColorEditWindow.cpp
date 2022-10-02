#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

void BrowEdit::showColorEditWindow()
{
	ImGui::Begin("Color Edit Window");

	static glm::vec4 refColor = colorEditBrushColor;
	ImGui::ColorPicker4("Color", glm::value_ptr(colorEditBrushColor), ImGuiColorEditFlags_AlphaBar, glm::value_ptr(refColor));
	if(ImGui::IsItemDeactivated())
		refColor = colorEditBrushColor;
	ImGui::SliderFloat("Brush Hardness", &colorEditBrushHardness, 0, 1);
	ImGui::InputInt("Brush Size", &colorEditBrushSize);
	ImGui::InputFloat("Brush Delay", &colorEditDelay, 0.05f);

	ImGui::Button("Picker");


	if (ImGui::CollapsingHeader("Map Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char buf[100];
		ImGui::InputText("##", buf, 100);
		ImGui::SameLine();
		toolBarButton("Save color", ICON_SAVE, "Saves this color as a preset", ImVec4(1, 1, 1, 1));


		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		ImGuiStyle& style = ImGui::GetStyle();
		for (int i = 0; i < 10; i++)
		{
			static bool bla = true;
			char label[32];
			sprintf_s(label, 32, "Item %d", i);
			ImGui::TableNextColumn();
			ImGui::PushID(i);
			if (ImGui::ColorButton("##", ImVec4(1, 0, 0, 1), 0, ImVec2(30, 30)))
			{

			}
			ImGui::PopID();

			float last_button_x2 = ImGui::GetItemRectMax().x;
			float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 30;
			if (next_button_x2 < window_visible_x2)
				ImGui::SameLine();

		}
	}
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Global Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
	}



	ImGui::End();
}