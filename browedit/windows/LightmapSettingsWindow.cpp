#include <browedit/BrowEdit.h>
#include <browedit/Lightmapper.h>

void BrowEdit::showLightmapSettingsWindow()
{
	if (windowData.openLightmapSettings)
	{
		ImGui::OpenPopup("Lightmap Settings");
		windowData.openLightmapSettings = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Lightmap Settings"))
	{
		ImGui::DragInt("Quality", &lightmapper->quality, 1, 1, 10);
		ImGui::Checkbox("Floor Shadows", &lightmapper->floorShadows);
		ImGui::Checkbox("Wall Shadows", &lightmapper->wallShadows);
		ImGui::Checkbox("Object Shadows", &lightmapper->objectShadows);
		ImGui::Checkbox("Global lighting", &lightmapper->globalLighting);
		ImGui::DragInt("Thread Count", &lightmapper->threadCount, 1, 1, 32);

		if (ImGui::Button("Lightmap!"))
		{
			lightmapper->begin();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
