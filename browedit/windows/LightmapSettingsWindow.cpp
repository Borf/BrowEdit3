#include <browedit/BrowEdit.h>
#include <browedit/Lightmapper.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <glm/gtc/type_ptr.hpp>

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
		auto gnd = lightmapper->map->rootNode->getComponent<Gnd>();
		auto rsw = lightmapper->map->rootNode->getComponent<Rsw>();
		auto& settings = rsw->lightmapSettings;
		ImGui::DragInt("Quality", &settings.quality, 1, 1, 10);
		ImGui::Checkbox("Sunlight", &settings.sunLight);
		ImGui::Checkbox("Shadows", &settings.shadows);
		ImGui::Checkbox("Diffuse Lighting", &settings.diffuseLighting);
		ImGui::Checkbox("Debug Points", &lightmapper->buildDebugPoints);
		ImGui::Checkbox("Height Edit Mode Selection Only", &settings.heightSelectionOnly);
		ImGui::DragInt2("Generate Range X", glm::value_ptr(settings.rangeX), 1, 0, gnd->width);
		ImGui::DragInt2("Generate Range Y", glm::value_ptr(settings.rangeY), 1, 0, gnd->height);
		ImGui::DragInt("Thread Count", &config.lightmapperThreadCount, 1, 1, 32);

		if (ImGui::Button("Lightmap!"))
		{
			config.save();
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
