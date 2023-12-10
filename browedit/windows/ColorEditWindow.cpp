#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

glm::vec4 refColor;
bool dropperEnabled = false;
void BrowEdit::showColorEditWindow()
{
	if (!activeMapView)
		return;
	ImGui::Begin("Color Edit Window");

	ImGui::ColorPicker4("Color", glm::value_ptr(colorEditBrushColor), ImGuiColorEditFlags_AlphaBar, glm::value_ptr(refColor));
	ImGui::SliderFloat("Brush Hardness", &colorEditBrushHardness, 0, 1);
	ImGui::InputInt("Brush Size", &colorEditBrushSize);

	if (toolBarButton("Color Picker", ICON_DROPPER, "Picks a color from the colormap", ImVec4(1, 1, 1, 1)))
	{
		dropperEnabled = !dropperEnabled;
		cursor = dropperEnabled ? dropperCursor : nullptr;
	}

	auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();

	ImGui::Spacing();

	auto buildTemplate = [&](const char* title, std::map<std::string, std::map<std::string, glm::vec4>>& data)
	{
		ImGui::PushID(title);
		if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (toolBarButton("Save", ICON_SAVE, "Saves this color as a palette color", ImVec4(1,1,1,1)))
				ImGui::OpenPopup("SavePalette");
			if(ImGui::BeginPopup("SavePalette"))
			{
				static char category[100];
				static char name[100];
				ImGui::InputText("Category", category, 100);
				ImGui::InputText("Name", name, 100);
				if (ImGui::Button("Save"))
				{
					data[category][name] = colorEditBrushColor;
					if(&data == &config.colorPresets)
						config.save();
				}
				ImGui::EndPopup();
			}

			float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			ImGuiStyle& style = ImGui::GetStyle();
			for (auto cat = data.begin(); cat != data.end(); )
			{
				bool erasedCat = false;
				ImGui::PushID(&cat);
				if (!ImGui::CollapsingHeader((cat->first + "##").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					cat++;
					ImGui::PopID();
					continue;
				}
				for (auto it = cat->second.begin(); !erasedCat && it != cat->second.end(); )
				{
					bool erased = false;
					if (ImGui::ColorButton((it->first + "##").c_str(), ImVec4(it->second.r, it->second.g, it->second.b, it->second.a), 0, ImVec2(30, 30)))
					{
						colorEditBrushColor = it->second;
					}
					if (ImGui::BeginPopupContextItem("Popup"))
					{
						if (ImGui::MenuItem("Delete"))
						{
							it = cat->second.erase(it);
							erased = true;
							if (cat->second.size() == 0)
							{
								cat = data.erase(cat);
								erasedCat = true;
								if (&data == &config.colorPresets)
									config.save();
							}
						}
						ImGui::EndPopup();
					}
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + 30;
					if (next_button_x2 < window_visible_x2)
						ImGui::SameLine();
					if (!erased)
						it++;
				}
				ImGui::Spacing();
				ImGui::PopID();
				if(!erasedCat)
					cat++;
			}
		}
		ImGui::PopID();
	};


	buildTemplate("Global Colors", config.colorPresets);
	buildTemplate("Map Colors", rsw->colorPresets);

	ImGui::End();
}