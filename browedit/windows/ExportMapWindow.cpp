#include <windows.h>
#include <browedit/BrowEdit.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/util/FileIO.h>
#include <iostream>

void BrowEdit::showExportWindow()
{
	if (windowData.exportVisible)
		ImGui::OpenPopup("Export Map");
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Export Map", 0, ImGuiWindowFlags_NoDocking))
	{
		ImGui::InputText("Export folder", &windowData.exportFolder);

		if (ImGui::BeginTable("Data", 3, ImGuiTableFlags_ScrollY))
		{
			for (auto& e : windowData.exportToExport)
			{
				ImGui::Checkbox(e.filename.c_str(), &e.enabled);
				ImGui::TableNextColumn();
				ImGui::Text(e.type.c_str());
				ImGui::TableNextColumn();
				ImGui::Text(e.source.c_str());
				ImGui::TableNextRow();
			}

			ImGui::EndTable();
		}
		ImGui::EndListBox();

		ImGui::EndPopup();
	}
}


void BrowEdit::exportMap(Map* map)
{
	windowData.exportVisible = true;
	windowData.exportMap = map;
	windowData.exportToExport.clear();
	ImGui::OpenPopup("Export Map");
}