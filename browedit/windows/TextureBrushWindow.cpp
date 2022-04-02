#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/MapView.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/Action.h>
#include <browedit/gl/Texture.h>

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




		ImGui::End();
	}
}
