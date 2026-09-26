#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/LubSkyMap.h>
#include <browedit/components/SkyMapRenderer.h>
#include <browedit/actions/SkyMapChangeAction.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

bool skyMapDropperEnabled = false;
void BrowEdit::showSkyMapEditWindow()
{
	if (!activeMapView)
		return;

	ImGui::Begin("SkyMap Edit Window");

	auto map = activeMapView->map;
	auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();
	auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
	auto rsw = map->rootNode->getComponent<Rsw>();

	if (rsw->version < 0x201) {
		ImGui::Text("SkyMap requires RSW version 0x201 or above");
		ImGui::End();
		return;
	}

	util::Checkbox(this, map, lubSkyMap->node, "Enabled (delete skymap if unchecked)", &lubSkyMap->isEnabled);
	util::ColorEdit3(this, map, lubSkyMap->node, "Background", &lubSkyMap->BG_Color);
	util::Checkbox(this, map, lubSkyMap->node, "BG_Fog", &lubSkyMap->BG_Fog);
	
	if (util::Checkbox(this, map, lubSkyMap->node, "Star effect", &lubSkyMap->Star_Effect)) {
		skyMapRenderer->setDirty();
	}

	if (ImGui::TreeNodeEx("Custom clouds", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		for (int i = 0; i < lubSkyMap->clouds.size(); i++)
		{
			ImGui::PushID(i);

			if (ImGui::TreeNodeEx("Cloud", ImGuiTreeNodeFlags_Framed))
			{
				if (ImGui::Button("Up"))
				{
					if (i > 0) {
						auto oldValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
						auto newValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
						std::swap(newValues[i], newValues[i - 1]);

						map->doAction(new SkyMapCustomCloudListChangedAction(oldValues, newValues), this);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Down"))
				{
					if (i < lubSkyMap->oldClouds.size() - 1) {
						auto oldValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
						auto newValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
						std::swap(newValues[i], newValues[i + 1]);

						map->doAction(new SkyMapCustomCloudListChangedAction(oldValues, newValues), this);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Dup"))
				{
					auto cloud = new LubSkyMap::CloudEffect(lubSkyMap->clouds[i]);
					map->doAction(new AddCustomCloudAction(cloud), this);
				}
				ImGui::SameLine();
				if (ImGui::Button("Del"))
				{
					auto oldValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
					auto newValues = std::vector<LubSkyMap::CloudEffect*>(lubSkyMap->clouds);
					newValues.erase(newValues.begin() + i);

					map->doAction(new SkyMapCustomCloudListChangedAction(oldValues, newValues), this);
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}

				util::DragInt(this, map, lubSkyMap->node, "Amount", &lubSkyMap->clouds[i]->Num, 1, 0, 10000);
				util::ColorEdit3(this, map, lubSkyMap->node, "Color", &lubSkyMap->clouds[i]->Color);
				util::DragInt(this, map, lubSkyMap->node, "CullDist", &lubSkyMap->clouds[i]->CullDist, 1, 0, 10000);
				util::DragFloat(this, map, lubSkyMap->node, "Size", &lubSkyMap->clouds[i]->Size, 0.1f, 0.1f, 100.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Size_Extra", &lubSkyMap->clouds[i]->Size_Extra, 0.1f, 0.0f, 200.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Expand_Rate", &lubSkyMap->clouds[i]->Expand_Rate, 0.01f, 0.0f, 10.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Inc_Time", &lubSkyMap->clouds[i]->Alpha_Inc_Time, 1.0f, 0.0f, 1000.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Inc_Time_Extra", &lubSkyMap->clouds[i]->Alpha_Inc_Time_Extra, 1.0f, 0.0f, 1000.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Inc_Speed", &lubSkyMap->clouds[i]->Alpha_Inc_Speed, 0.01f, 0.0f, 20.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Dec_Time", &lubSkyMap->clouds[i]->Alpha_Dec_Time, 1.0f, 0.0f, 1000.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Dec_Time_Extra", &lubSkyMap->clouds[i]->Alpha_Dec_Time_Extra, 1.0f, 0.0f, 1000.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Alpha_Dec_Speed", &lubSkyMap->clouds[i]->Alpha_Dec_Speed, 0.01f, 0.0f, 20.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Height", &lubSkyMap->clouds[i]->Height, 0.1f, -300.0f, 300.0f);
				util::DragFloat(this, map, lubSkyMap->node, "Height_Extra", &lubSkyMap->clouds[i]->Height_Extra, 0.1f, -300.0f, 300.0f);

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (ImGui::Button("Add new"))
		{
			auto cloud = new LubSkyMap::CloudEffect();
			auto gnd = map->rootNode->getComponent<Gnd>();
			cloud->NumPerSquared = (float)(60.0f * 4 / glm::pow(400, 2));
			cloud->Num = (int)(cloud->NumPerSquared * gnd->width * gnd->height * 100.0f);

			map->doAction(new AddCustomCloudAction(cloud), this);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Old clouds", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		for (int i = 0; i < lubSkyMap->oldClouds.size(); i++)
		{
			ImGui::PushID(i);
			if (util::DragInt(this, map, lubSkyMap->node, "Type", &lubSkyMap->oldClouds[i])) {
				skyMapRenderer->setDirty();
			}
			ImGui::SameLine();
			if (ImGui::Button("Up"))
			{
				if (i > 0) {
					auto oldValues = std::vector<int>(lubSkyMap->oldClouds);
					auto newValues = std::vector<int>(lubSkyMap->oldClouds);
					std::swap(newValues[i], newValues[i - 1]);

					map->doAction(new SkyMapOldCloudListChangedAction(oldValues, newValues), this);
				}	
			}
			ImGui::SameLine();
			if (ImGui::Button("Down"))
			{
				if (i < lubSkyMap->oldClouds.size() - 1) {
					auto oldValues = std::vector<int>(lubSkyMap->oldClouds);
					auto newValues = std::vector<int>(lubSkyMap->oldClouds);
					std::swap(newValues[i], newValues[i + 1]);

					map->doAction(new SkyMapOldCloudListChangedAction(oldValues, newValues), this);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Del"))
			{
				auto oldValues = std::vector<int>(lubSkyMap->oldClouds);
				auto newValues = std::vector<int>(lubSkyMap->oldClouds);
				newValues.erase(newValues.begin() + i);

				map->doAction(new SkyMapOldCloudListChangedAction(oldValues, newValues), this);
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Add new"))
		{
			auto oldValues = std::vector<int>(lubSkyMap->oldClouds);
			auto newValues = std::vector<int>(lubSkyMap->oldClouds);
			newValues.push_back(1);

			map->doAction(new SkyMapOldCloudListChangedAction(oldValues, newValues), this);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}