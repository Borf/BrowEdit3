#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <glm/gtc/type_ptr.hpp>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/GatRenderer.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/WaterRenderer.h>

void BrowEdit::showCropSettingsWindow()
{
	if (activeMapView == nullptr)
		return;

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::Begin("Crop Settings");

	auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
	auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();

	if (gnd == nullptr || rsw == nullptr)
		return;

	auto& settings = rsw->cropSettings;

	std::string info = "Current map size: " + std::to_string(gnd->width) + " / " + std::to_string(gnd->height);
	ImGui::Text(info.c_str());
	ImGui::Checkbox("Height Edit Mode Selection Only", &settings.heightSelectionOnly);
	ImGui::DragInt("Crop left", &settings.cropLeft, 1, -gnd->width, gnd->width);
	ImGui::DragInt("Crop right", &settings.cropRight, 1, -gnd->width, gnd->width);
	ImGui::DragInt("Crop top", &settings.cropTop, 1, -gnd->height, gnd->height);
	ImGui::DragInt("Crop bottom", &settings.cropBottom, 1, -gnd->height, gnd->height);
	
	if (ImGui::Button("Crop!"))
	{
		auto gat = activeMapView->map->rootNode->getComponent<Gat>();
		std::vector<std::vector<Gnd::Cube*>> cubes;
		std::vector<std::vector<Gat::Cube*>> gatCubes;
		glm::ivec2 min(gnd->width, gnd->height);
		glm::ivec2 max(0);

		if (settings.heightSelectionOnly && activeMapView->map->tileSelection.size() == 0) {
			// Nothing selected!
		}
		else {
			auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();
			glm::ivec2 oldGndSize(gnd->width, gnd->height);
			glm::ivec2 oldGatSize(gat->width, gat->height);

			if (settings.heightSelectionOnly) {
				for (const auto& tile : activeMapView->map->tileSelection)
				{ //TODO: can this be done with 2 min calls?
					min.x = glm::min(min.x, tile.x);
					min.y = glm::min(min.y, tile.y);
					max.x = glm::max(max.x, tile.x);
					max.y = glm::max(max.y, tile.y);
				}
			}
			else {
				min = glm::ivec2(0);
				max = glm::ivec2(gnd->width - 1, gnd->height - 1);

				min.x += settings.cropLeft;
				min.y += settings.cropBottom;
				max.x -= settings.cropRight;
				max.y -= settings.cropTop;
			}

			max.x++;
			max.y++;

			if (min.x < max.x && min.y < max.y) {
				glm::vec3 cameraOffset(0);
				glm::vec3 rswObjectOffset(0);

				cameraOffset.x = min.x * 10.0f;
				cameraOffset.z = (gnd->height - max.y) * 10.0f;

				rswObjectOffset.x = -min.x * 10.0f + 5 * gnd->width;
				rswObjectOffset.z = (gnd->height - max.y) * 10.0f - 5 * gnd->height;

				gnd->width = max.x - min.x;
				gnd->height = max.y - min.y;

				rswObjectOffset.x -= 5 * gnd->width;
				rswObjectOffset.z += 5 * gnd->height;

				for (int x = min.x; x < max.x; x++)
				{
					std::vector<Gnd::Cube*> row;
					for (int y = min.y; y < max.y; y++)
					{
						if (x < 0 || x >= oldGndSize.x || y < 0 || y >= oldGndSize.y)
							row.push_back(new Gnd::Cube());
						else
							row.push_back(gnd->cubes[x][y]);
					}
					cubes.push_back(row);
				}
				for (int x = min.x * 2; x < max.x * 2; x++)
				{
					std::vector<Gat::Cube*> row;
					for (int y = min.y * 2; y < max.y * 2; y++)
					{
						if (x < 0 || x >= oldGatSize.x || y < 0 || y >= oldGatSize.y) {
							auto gatCube = new Gat::Cube();
							gatCube->gatType = 0;
							gatCube->normal = glm::vec3(0, -1, 0);
							row.push_back(gatCube);
						}
						else
							row.push_back(gat->cubes[x][y]);
					}

					gatCubes.push_back(row);
				}

				gnd->cubes = cubes;
				gnd->width = glm::max(1, max.x - min.x);
				gnd->height = glm::max(1, max.y - min.y);
				glm::vec3 newCenterOffset(5 * -gnd->width, 0, 5 * -gnd->height);

				if (!settings.heightSelectionOnly && (settings.cropLeft < 0 || settings.cropBottom < 0 || settings.cropRight < 0 || settings.cropTop < 0))
					gndRenderer->rebuildChunks();
				else
					gndRenderer->setChunksDirty();

				activeMapView->map->tileSelection.clear();
				activeMapView->map->rootNode->getComponent<WaterRenderer>()->setDirty();

				gat->cubes = gatCubes;
				gat->width = 2 * (max.x - min.x);
				gat->height = 2 * (max.y - min.y);
				activeMapView->map->rootNode->getComponent<GatRenderer>()->setChunksDirty();

				activeMapView->map->rootNode->traverse([&rswObjectOffset](Node* n)
					{
						auto rsmRenderer = n->getComponent<RsmRenderer>();
						if (rsmRenderer) {
							rsmRenderer->setDirty();
						}
						auto rswObject = n->getComponent<RswObject>();
						if (rswObject) {
							rswObject->position += rswObjectOffset;
						}
					});

				// only moves for left crop or top crop
				activeMapView->cameraCenter -= cameraOffset;

				auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();
				delete rsw->quadtree;
				rsw->quadtree = new Rsw::QuadTreeNode(10 * -gnd->width / 2.0f, 10 * -gnd->height / 2.0f, (float)gnd->width * 10 - 1.0f, (float)gnd->height * 10 - 1.0f, 0);
				rsw->recalculateQuadtree(nullptr);
			}
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		windowData.cropWindowVisible = false;
	}
	
	ImGui::End();
}
