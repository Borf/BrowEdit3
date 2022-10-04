#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

glm::vec4 refColor;
void BrowEdit::showColorEditWindow()
{
	if (!activeMapView)
		return;
	ImGui::Begin("Color Edit Window");

	ImGui::ColorPicker4("Color", glm::value_ptr(colorEditBrushColor), ImGuiColorEditFlags_AlphaBar, glm::value_ptr(refColor));
	ImGui::SliderFloat("Brush Hardness", &colorEditBrushHardness, 0, 1);
	ImGui::InputInt("Brush Size", &colorEditBrushSize);
	ImGui::InputFloat("Brush Delay", &colorEditDelay, 0.05f);

	ImGui::Button("Picker");

	auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();

	ImGui::PushID("MapColor");
	if (ImGui::CollapsingHeader("Map Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::CollapsingHeader("Saving", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char category[100];
			static char name[100];
			ImGui::InputText("Category", category, 100);
			ImGui::InputText("Name", name, 100);
			if (ImGui::Button("Save"))
				rsw->colorPresets[category][name] = colorEditBrushColor;
			ImGui::Spacing();
		}


		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		ImGuiStyle& style = ImGui::GetStyle();
		for(const auto cat : rsw->colorPresets)
		{
			if (!ImGui::CollapsingHeader((cat.first + "##").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				continue;
			for (const auto color : cat.second)
			{
				if (ImGui::ColorButton((color.first + "##").c_str(), ImVec4(color.second.r, color.second.g, color.second.b, color.second.a), 0, ImVec2(30, 30)))
				{
					colorEditBrushColor = color.second;
				}
				float last_button_x2 = ImGui::GetItemRectMax().x;
				float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 30;
				if (next_button_x2 < window_visible_x2)
					ImGui::SameLine();
			}
			ImGui::Spacing();

		}
	}
	ImGui::PopID();
	ImGui::Spacing();

	ImGui::PushID("Global Colors");
	if (ImGui::CollapsingHeader("Global Colors", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::CollapsingHeader("Saving", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char category[100];
			static char name[100];
			ImGui::InputText("Category", category, 100);
			ImGui::InputText("Name", name, 100);
			if (ImGui::Button("Save"))
			{
				config.colorPresets[category][name] = colorEditBrushColor;
				config.save();
			}
			ImGui::Spacing();
		}

		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		ImGuiStyle& style = ImGui::GetStyle();
		for (const auto cat : config.colorPresets)
		{
			if (!ImGui::CollapsingHeader((cat.first + "##").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				continue;
			for (const auto color : cat.second)
			{
				if (ImGui::ColorButton((color.first + "##").c_str(), ImVec4(color.second.r, color.second.g, color.second.b, color.second.a), 0, ImVec2(30, 30)))
				{
					colorEditBrushColor = color.second;
				}
				float last_button_x2 = ImGui::GetItemRectMax().x;
				float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 30;
				if (next_button_x2 < window_visible_x2)
					ImGui::SameLine();
			}
			ImGui::Spacing();

		}
	}
	ImGui::PopID();


	ImGui::End();
}