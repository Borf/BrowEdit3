#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/components/Gat.h>
#include <browedit/components/GatRenderer.h>
#include <browedit/components/Rsw.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/gl/Texture.h>
#include <browedit/actions/CubeHeightChangeAction.h>

extern std::string gatTypes[];
extern ImVec4 enabledColor;// (144 / 255.0f, 193 / 255.0f, 249 / 255.0f, 0.5f);
extern ImVec4 disabledColor;// (72 / 255.0f, 96 / 255.0f, 125 / 255.0f, 0.5f);
extern std::vector<std::vector<glm::vec3>> debugPoints;

void BrowEdit::showGatWindow()
{
	ImGui::Begin("Gat Edit");
	if (!activeMapView)
	{
		ImGui::End();
		return;
	}

	auto& map = activeMapView->map;
	auto gat = map->rootNode->getComponent<Gat>();
	auto gatRenderer = map->rootNode->getComponent<GatRenderer>();

	ImGui::PushItemWidth(-200);

	if (windowData.gatEdit.doodle && heightDoodle)
		windowData.gatEdit.doodle = false;

	if (ImGui::TreeNodeEx("Tool", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		ImGuiStyle& style = ImGui::GetStyle();
		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		for (int i = 0; i < 16; i++)
		{
			ImVec2 v1(.25f * (i % 4), .25f * (i / 4));
			ImVec2 v2(v1.x + .25f, v1.y + .25f);
			ImGui::PushID(i);
			bool clicked = ImGui::ImageButton((ImTextureID)(long long)gatTexture->id(), ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, windowData.gatEdit.gatIndex == i ? enabledColor : disabledColor);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(gatTypes[i].c_str());

			float last_button_x2 = ImGui::GetItemRectMax().x;
			float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.toolbarButtonSize;


			ImGui::PopID();
			if (clicked)
			{
				windowData.gatEdit.gatIndex = i;
				heightDoodle = false;
				windowData.gatEdit.doodle = true;
			}
			if (i < 15 && next_button_x2 < window_visible_x2)
				ImGui::SameLine();
		}
		if (toolBarToggleButton("Height", ICON_HEIGHT_DOODLE, heightDoodle, "Height editing", ImVec4(1, 1, 1, 1)))
		{
			heightDoodle = true;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("Rectangle", ICON_SELECT_RECTANGLE, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::Rectangle, "Rectangle Select", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::Rectangle;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("Lasso", ICON_SELECT_LASSO, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::Lasso, "Lasso Select", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::Lasso;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("WandTex", ICON_SELECT_WAND_TEX, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::WandTex, "Magic Wand (Texture)", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::WandTex;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("WandHeight", ICON_SELECT_WAND_HEIGHT, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::WandHeight, "Magic Wand (Height)", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::WandHeight;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("AllTex", ICON_SELECT_ALL_TEX, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::AllTex, "Select All (Texture)", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::AllTex;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::SameLine();
		if (toolBarToggleButton("AllHeight", ICON_SELECT_ALL_HEIGHT, !heightDoodle && !windowData.gatEdit.doodle && selectTool == BrowEdit::SelectTool::AllHeight, "Select All (Height)", ImVec4(1, 1, 1, 1)))
		{
			selectTool = BrowEdit::SelectTool::AllHeight;
			heightDoodle = false;
			windowData.gatEdit.doodle = false;
		}
		ImGui::TreePop();
	}

	if (!heightDoodle && !windowData.gatEdit.doodle)
	{
		ImGui::Checkbox("Paste gat types", &windowData.gatEdit.pasteType);
		ImGui::Checkbox("Paste gat heights", &windowData.gatEdit.pasteHeight);

		if (ImGui::TreeNodeEx("Selection Options", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			toolBarToggleButton("TriangleSplit", windowData.gatEdit.splitTriangleFlip ? ICON_HEIGHT_SPLIT_1 : ICON_HEIGHT_SPLIT_2, &windowData.gatEdit.splitTriangleFlip, "Change the way quads are split into triangles");
			ImGui::SameLine();
			toolBarToggleButton("CenterArrow", ICON_HEIGHT_SELECT_CENTER, &windowData.gatEdit.showCenterArrow, "Show Center Arrow");
			ImGui::SameLine();
			toolBarToggleButton("CornerArrow", ICON_HEIGHT_SELECT_CORNERS, &windowData.gatEdit.showCornerArrows, "Show Corner Arrows");
			ImGui::SameLine();
			toolBarToggleButton("EdgeArrow", ICON_HEIGHT_SELECT_SIDES, &windowData.gatEdit.showEdgeArrows, "Show Edge Arrows");


			if (ImGui::TreeNodeEx("Edge handling", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
			{
				ImGui::RadioButton("Nothing", &windowData.gatEdit.edgeMode, 0);
				ImGui::SameLine();
				ImGui::RadioButton("Raise ground", &windowData.gatEdit.edgeMode, 1);
				ImGui::RadioButton("Snap ground", &windowData.gatEdit.edgeMode, 2);
				ImGui::TreePop();
			}

			//if (ImGui::Button("Grow selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
			//	map->growGatSelection(browEdit);
			//if (ImGui::Button("Shrink selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
			//	map->shrinkGatSelection(browEdit);

			ImGui::TreePop();
		}
	}
	else if (heightDoodle && !windowData.gatEdit.doodle)
	{
		if (ImGui::TreeNodeEx("Brush Options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragInt("Brush Size", &windowData.gatEdit.doodleSize, 1.0f, 0, 10);
			ImGui::DragFloat("Brush Hardness", &windowData.gatEdit.doodleHardness, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("Brush Speed", &windowData.gatEdit.doodleSpeed, 0.01f, 0, 1);

			ImGui::TreePop();
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
				windowData.gatEdit.doodleSize++;
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
				windowData.gatEdit.doodleSize = glm::max(windowData.gatEdit.doodleSize - 1, 0);
		}
	}
	else if (windowData.gatEdit.doodle && !heightDoodle)
	{
		if (ImGui::TreeNodeEx("Brush Options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragInt("Brush Size", &windowData.gatEdit.doodleSize, 1.0f, 0, 10);
			ImGui::TreePop();
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
				windowData.gatEdit.doodleSize++;
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
				windowData.gatEdit.doodleSize = glm::max(windowData.gatEdit.doodleSize - 1, 0);
		}
	}
	if (ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		/*	ImGui::BeginGroup();
			if (ImGui::Button("Smooth selection", ImVec2(ImGui::GetContentRegionAvailWidth() - 12 - config.toolbarButtonSize, 0)))
				gnd->smoothTiles(map, browEdit, map->gatSelection, 3);
			ImGui::SameLine();
			toolBarToggleButton("Connect", ICON_NOICON, false, "Connects tiles around selection");
			ImGui::EndGroup();
			if (ImGui::Button("Smooth selection Horizontal", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(map, browEdit, map->gatSelection, 1);
			if (ImGui::Button("Smooth selection Vertical", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(map, browEdit, map->gatSelection, 2);
			if (ImGui::Button("Flatten selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->flattenTiles(map, browEdit, map->gatSelection);
			if (ImGui::Button("Connect tiles high", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectHigh(map, browEdit, map->gatSelection);
			if (ImGui::Button("Connect tiles low", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectLow(map, browEdit, map->gatSelection);

			if(ImGui::TreeNodeEx("Set Height", ImGuiTreeNodeFlags_Framed))
			{
				static float height = 0;
				ImGui::InputFloat("Height", &height);
				if (ImGui::Button("Set selection to height"))
				{
					auto action = new CubeHeightChangeAction(gnd, map->gatSelection);
					for (auto t : map->gatSelection)
						for(int i = 0; i < 4; i++)
							gnd->cubes[t.x][t.y]->heights[i] = height;

					action->setNewHeights(gnd, map->gatSelection);
					map->doAction(action, browEdit);
				}
				ImGui::TreePop();
			}*/

		static bool selectionOnly = false;
		static bool setWalkable = true;
		static bool setHeight = true;
		static bool setObjectWalkable = true;
		static bool blockUnderWater = true;
		static std::vector<int> textureMap; //TODO: this won't work well with multiple maps
		auto gnd = map->rootNode->getComponent<Gnd>();
		if (textureMap.size() != gnd->textures.size())
			textureMap.resize(gnd->textures.size(), -1);

		if (ImGui::Button("AutoGat"))
		{
			windowData.progressWindowVisible = true;
			windowData.progressWindowProgres = 0;
			windowData.progressWindowText = "Calculating gat";

			std::thread t([this, gnd, gat, map, gatRenderer]()
				{
					debugPoints.clear();
					debugPoints.resize(2);

					std::map<int, std::map<int, std::pair<int, int>>> gatInfo;

					auto rsw = map->rootNode->getComponent<Rsw>();
					for (int x = 0; x < gat->width; x++)
					{
						for (int y = 0; y < gat->height; y++)
						{
							if (selectionOnly && std::find(map->gatSelection.begin(), map->gatSelection.end(), glm::ivec2(x, y)) == map->gatSelection.end())
								continue;

							windowData.progressWindowProgres = 0.5f * (x / (float)gat->width) + (1.0f / gat->width) * (y / (float)gat->height);

							int raiseType = -1;
							int gatType = -1;

							for (int i = 0; i < 4; i++)
							{
								glm::vec3 pos = gat->getPos(x, y, i, 0.01f);

								//debugPoints[0].push_back(pos);
								pos.y = 1000;
								math::Ray ray(pos, glm::vec3(0, -1, 0));
								auto height = gnd->rayCast(ray, true, x / 2 - 2, y / 2 - 2, x / 2 + 2, y / 2 + 2);
								if (height.y > 100000 || height.y < -100000)
								{
									std::cout << "wtf" << std::endl;
									height = gnd->rayCast(ray, true);// , x / 2 - 2, y / 2 - 2, x / 2 + 2, y / 2 + 2);
								}
								//debugPoints[1].push_back(height);

								map->rootNode->traverse([&](Node* n) {
									auto rswModel = n->getComponent<RswModel>();
									if (rswModel)
									{
										auto collider = n->getComponent<RswModelCollider>();
										auto collisions = collider->getCollisions(ray);
										if (rswModel->gatCollision)
										{
											for (const auto& c : collisions)
												height.y = glm::max(height.y, c.y);
											if (collisions.size() > 0 && rswModel->gatStraightType > 0)
												raiseType = rswModel->gatStraightType;
										}
										if (rswModel->gatType > -1 && collisions.size() > 0)
										{
											gatType = rswModel->gatType;
										}
									}
									});
								if(setHeight)
									gat->cubes[x][y]->heights[i] = -height.y;
							}

							gatInfo[x][y] = std::pair<int, int>(raiseType, gatType);
						}
					}

					for (int x = 0; x < gat->width; x++)
					{
						for (int y = 0; y < gat->height; y++)
						{
							if (selectionOnly && std::find(map->gatSelection.begin(), map->gatSelection.end(), glm::ivec2(x, y)) == map->gatSelection.end())
								continue;

							windowData.progressWindowProgres = 0.5f + 0.5f * (x / (float)gat->width) + (1.0f / gat->width) * (y / (float)gat->height);

							int raiseType = gatInfo[x][y].first;
							int gatType = gatInfo[x][y].second;

							if (setWalkable)
							{
								gat->cubes[x][y]->calcNormal();
								gat->cubes[x][y]->gatType = 0;
								float angle = glm::dot(gat->cubes[x][y]->normal, glm::vec3(0, -1, 0));
								if (angle < 0.9)
									gat->cubes[x][y]->gatType = 1;
								else
								{
									bool stepHeight = false;
									glm::ivec2 t(x, y);
									for (int i = 0; i < 4; i++)
									{
										for (int ii = 0; ii < 4; ii++)
										{
											auto& connectInfo = gat->connectInfo[i][ii];
											glm::ivec2 tt = t + glm::ivec2(connectInfo.x, connectInfo.y);
											if (!gat->inMap(tt))
												continue;
											if (gat->cubes[x][y]->heights[i] - gat->cubes[tt.x][tt.y]->heights[connectInfo.z] > 5)
												stepHeight = true;
										}
									}
									if (stepHeight)
										gat->cubes[x][y]->gatType = 1;

								}
							}
							if (blockUnderWater)
							{
								bool underWater = false;
								for (int i = 0; i < 4; i++) {
									auto water = rsw->water.getFromGat(x, y, gnd);

									if (gat->cubes[x][y]->heights[i] > water->height)
										underWater = true;
								}
								if (underWater)
									gat->cubes[x][y]->gatType = 1;
							}
							if (gatType > -1 && setObjectWalkable)
								gat->cubes[x][y]->gatType = gatType;
							if (raiseType > -1)
							{
								for (int i = 1; i < 4; i++)
								{
									if (raiseType == 1)
										gat->cubes[x][y]->heights[0] = glm::min(gat->cubes[x][y]->heights[0], gat->cubes[x][y]->heights[i]);
									else if (raiseType == 2)
										gat->cubes[x][y]->heights[0] = glm::max(gat->cubes[x][y]->heights[0], gat->cubes[x][y]->heights[i]);
								}
								for (int i = 1; i < 4; i++)
									gat->cubes[x][y]->heights[i] = gat->cubes[x][y]->heights[0];
							}
						}
					}

					windowData.progressWindowProgres = 1;
					windowData.progressWindowVisible = false;

					gatRenderer->allDirty = true;
				});
			t.detach();
		}
		ImGui::Checkbox("Gat selection only", &selectionOnly);
		ImGui::Checkbox("Set walkability", &setWalkable);
		ImGui::Checkbox("Set height", &setHeight);
		ImGui::Checkbox("Set object walkability", &setObjectWalkable);
		ImGui::Checkbox("Make tiles under water unwalkable", &blockUnderWater);

		if (ImGui::Button("WaterGat"))
		{
			windowData.progressWindowVisible = true;
			windowData.progressWindowProgres = 0;
			windowData.progressWindowText = "Calculating watergat";

			std::thread t([this, gnd, gat, map, gatRenderer]()
				{
					auto rsw = map->rootNode->getComponent<Rsw>();
					for (int x = 0; x < gat->width; x++)
					{
						for (int y = 0; y < gat->height; y++)
						{
							if (selectionOnly && std::find(map->gatSelection.begin(), map->gatSelection.end(), glm::ivec2(x, y)) == map->gatSelection.end())
								continue;
							windowData.progressWindowProgres = (x / (float)gat->width) + (1.0f / gat->width) * (y / (float)gat->height);
							bool underWater = false;
							for (int i = 0; i < 4; i++)
							{
								glm::vec3 pos = gat->getPos(x, y, i, 0.025f);
								pos.y = 1000;
								math::Ray ray(pos, glm::vec3(0, -1, 0));
								auto height = gnd->rayCast(ray, true, x / 2 - 2, y / 2 - 2, x / 2 + 2, y / 2 + 2);
								map->rootNode->traverse([&](Node* n) {
									auto rswModel = n->getComponent<RswModel>();
									if (rswModel && rswModel->gatCollision)
									{
										auto collider = n->getComponent<RswModelCollider>();
										auto collisions = collider->getCollisions(ray);
										for (const auto& c : collisions)
											height.y = glm::max(height.y, c.y);
									}
									});

								auto water = rsw->water.getFromGat(x, y, gnd);
								gat->cubes[x][y]->heights[i] = water->height;
								underWater |= height.y < -water->height;
							}
							gat->cubes[x][y]->gatType = underWater ? 0 : 1;
						}
					}
					windowData.progressWindowProgres = 1;
					windowData.progressWindowVisible = false;

					gatRenderer->allDirty = true;
				});
			t.detach();
		}

		ImGui::TreePop();
	}

	if (!heightDoodle)
	{
		if (ImGui::TreeNodeEx("Set Height", ImGuiTreeNodeFlags_Framed))
		{
			static float height = 0;
			ImGui::InputFloat("Height", &height);
			if (ImGui::Button("Set selection to height"))
			{
				auto action = new CubeHeightChangeAction<Gat, Gat::Cube>(gat, activeMapView->map->tileSelection);
				for (auto t : activeMapView->map->gatSelection)
					for (int i = 0; i < 4; i++)
						gat->cubes[t.x][t.y]->heights[i] = height;

				action->setNewHeights(gat, activeMapView->map->gatSelection);
				activeMapView->map->doAction(action, this);
			}
			ImGui::TreePop();
		}
	}
	/*if (map->gatSelection.size() > 0 && !heightDoodle)
	{
		if (map->gatSelection.size() > 1)
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "WARNING, THESE SETTINGS ONLY\nWORK ON THE FIRST TILE");
		auto cube = gat->cubes[map->gatSelection[0].x][map->gatSelection[0].y];
		if (ImGui::TreeNodeEx("Tile Details", ImGuiTreeNodeFlags_Framed))
		{
			bool changed = false;
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H1", &cube->h1);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H2", &cube->h2);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H3", &cube->h3);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H4", &cube->h4);


			if (changed)
				gatRenderer->setChunkDirty(map->gatSelection[0].x, map->gatSelection[0].y);
			ImGui::TreePop();
		}
	}*/
	ImGui::PopItemWidth();
	ImGui::End();
}