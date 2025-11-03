#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <browedit/HotkeyRegistry.h>
#include <browedit/actions/WaterSplitChangeAction.h>
#include <browedit/util/Util.h>
#include <GLFW/glfw3.h>

#include <browedit/components/WaterRenderer.h>

template bool util::DragIntMulti<Rsw::Water>(
	BrowEdit* browEdit,
	Map* map,
	const std::vector<Rsw::Water*>& data,
	const char* label,
	const std::function<int* (Rsw::Water*)>& callback,
	int v_speed,
	int v_min,
	int v_max);

void BrowEdit::showWaterEditWindow()
{
	if (!activeMapView)
	{
		return;
	}

	static bool showWaterOverlay = true;

	ImGui::Begin("Water Edit");

	ImGui::Text("Water zones require GND version 0x108 or above");
	ImGui::Checkbox("Show grid", &this->activeMapView->showWaterGrid);
	ImGui::Checkbox("Show selected overlay", &this->activeMapView->showWaterSelectedOverlay);

	// Allow brushes in the future?
	//if (ImGui::TreeNodeEx("Tool", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	//{
	//	toolBarToggleButton("Rectangle", ICON_SELECT_RECTANGLE, !heightDoodle && selectTool == BrowEdit::SelectTool::Rectangle, "Rectangle Select", HotkeyAction::HeightEdit_Rectangle, ImVec4(1, 1, 1, 1));
	//	ImGui::TreePop();
	//}

	auto gnd = this->activeMapView->map->rootNode->getComponent<Gnd>();
	auto rsw = this->activeMapView->map->rootNode->getComponent<Rsw>();
	auto rswNode = rsw->node;
	auto water = &rsw->water;
	auto waterRenderer = rswNode->getComponent<WaterRenderer>();

	if (ImGui::TreeNodeEx("Water", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		if (gnd->version >= 0x108) {
			if (util::DragInt(this, this->activeMapView->map, rswNode, "Split width", &water->splitWidth, 1, 1, gnd->width, "", [&](int* ptr, int startValue) {
				water->splitWidth = glm::min(gnd->width, glm::max(1, water->splitWidth));
				water->splitHeight = glm::min(gnd->height, glm::max(1, water->splitHeight));
				auto action = new WaterSplitChangeAction(*water, startValue, water->splitHeight);
				this->activeMapView->map->doAction(action, this);
				})) {
				water->resize(water->splitWidth, water->splitHeight);
				waterRenderer->setDirty();
			}

			if (util::DragInt(this, this->activeMapView->map, rswNode, "Split height", &water->splitHeight, 1, 1, gnd->height, "", [&](int* ptr, int startValue) {
				water->splitWidth = glm::min(gnd->width, glm::max(1, water->splitWidth));
				water->splitHeight = glm::min(gnd->height, glm::max(1, water->splitHeight));
				auto action = new WaterSplitChangeAction(*water, glm::max(1, water->splitWidth), startValue);
				this->activeMapView->map->doAction(action, this);
				})) {
				water->resize(water->splitWidth, water->splitHeight);
				waterRenderer->setDirty();
			}

			if (rsw->water.splitWidth > 1 || rsw->water.splitHeight) {
				ImGui::Text("Zone width (in GND tile): %d", glm::max(1, (int)(gnd->width / glm::max(1, rsw->water.splitWidth))));
				ImGui::Text("Zone height (in GND tile): %d", glm::max(1, (int)(gnd->height / glm::max(1, rsw->water.splitHeight))));
			}
		}

		if (gnd->version >= 0x108) {
			if (activeMapView->map->waterSelection.size() == 0 && water->splitHeight * water->splitWidth == 1) {
				activeMapView->map->waterSelection.push_back(glm::ivec2(0, 0));
			}

			std::vector<Rsw::Water*> selectedZones;
		
			for (auto& tile : this->activeMapView->map->waterSelection) {
				// The selection can become invalid if the split zones count was changed
				if (tile.x < 0 || tile.x >= rsw->water.splitWidth ||
					tile.y < 0 || tile.y >= rsw->water.splitHeight)
					continue;

				// Cheat a little... copy the rsw's node so that the action will be able to redo/undo correctly.
				auto zone = &rsw->water.zones[tile.x][tile.y];
				zone->node = rswNode;
				selectedZones.push_back(zone);
			}

			if (gnd->version == 0x108) {
				if (util::DragInt(this, this->activeMapView->map, rswNode, "Type", &water->zones[0][0].type, 1, 0, 1000)) {
					waterRenderer->reloadTextures();
				}
			}

			if (selectedZones.size() > 0) {
				if (gnd->version >= 0x109) {
					if (util::DragIntMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Type", [](Rsw::Water* m) { return &m->type; }, 1, 0, 1000)) {
						waterRenderer->reloadTextures();
					}
				}

				// Height edit is shared for both 0x108 and 0x109+
				if (util::DragFloatMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Height", [](Rsw::Water* m) { return &m->height; }, 0.1f, -100, 100)) {
					if (!waterRenderer->renderFullWater) {
						waterRenderer->renderFullWater = true;
						waterRenderer->setDirty();
					}
				}
		
				if (gnd->version >= 0x109) {
					util::DragFloatMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Wave Height", [](Rsw::Water* m) { return &m->amplitude; }, 0.1f, -100, 100);
					util::DragIntMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Texture Animation Speed", [](Rsw::Water* m) { return &m->textureAnimSpeed; }, 1, 0, 1000);
					util::DragFloatMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Wave Speed", [](Rsw::Water* m) { return &m->waveSpeed; }, 0.1f, -100, 100);
					util::DragFloatMulti<Rsw::Water>(this, this->activeMapView->map, selectedZones, "Wave Pitch", [](Rsw::Water* m) { return &m->wavePitch; }, 0.1f, -100, 100);
				}
			}

			if (gnd->version == 0x108) {
				util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Height", &water->zones[0][0].amplitude, 0.1f, -100, 100);
				util::DragInt(this, this->activeMapView->map, rswNode, "Texture Animation Speed", &water->zones[0][0].textureAnimSpeed, 1, 0, 1000);
				util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Speed", &water->zones[0][0].waveSpeed, 0.1f, -100, 100);
				util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Pitch", &water->zones[0][0].wavePitch, 0.1f, -100, 100);
			}
		}
		else {
			if (util::DragInt(this, this->activeMapView->map, rswNode, "Type", &water->zones[0][0].type, 1, 0, 1000)) {
				waterRenderer->reloadTextures();
			}

			if (util::DragFloat(this, this->activeMapView->map, rswNode, "Height", &water->zones[0][0].height, 0.1f, -100, 100)) {
				if (!waterRenderer->renderFullWater) {
					waterRenderer->renderFullWater = true;
					waterRenderer->setDirty();
				}
			}

			util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Height", &water->zones[0][0].amplitude, 0.1f, -100, 100);
			util::DragInt(this, this->activeMapView->map, rswNode, "Texture Animation Speed", &water->zones[0][0].textureAnimSpeed, 1, 0, 1000);
			util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Speed", &water->zones[0][0].waveSpeed, 0.1f, -100, 100);
			util::DragFloat(this, this->activeMapView->map, rswNode, "Wave Pitch", &water->zones[0][0].wavePitch, 0.1f, -100, 100);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}