#include <windows.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/Lightmapper.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/HotkeyRegistry.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/GroupAction.h>
#include <GLFW/glfw3.h>
#include <imgui_internal.h>
#include <fstream>

void BrowEdit::menuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		hotkeyMenuItem("New", HotkeyAction::Global_New);
		hotkeyMenuItem("Open", HotkeyAction::Global_Load);
		if(activeMapView)
			hotkeyMenuItem("Save " + activeMapView->map->name, HotkeyAction::Global_Save);
		if (activeMapView && ImGui::MenuItem("Save as "))
			saveAsMap(activeMapView->map);
		if (activeMapView && ImGui::MenuItem("Export to folder"))
			exportMap(activeMapView->map);
		if (ImGui::BeginMenu("Recent Maps", config.recentFiles.size() > 0))
		{
			for (const auto& m : config.recentFiles)
				if (ImGui::MenuItem(m.c_str()))
				{
					loadMap(m);
					break;
				}
			ImGui::EndMenu();
		}
		if (activeMapView && ImGui::MenuItem("Export mapdata for lightmapping"))
		{
			nlohmann::json mapData;
			auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();
			auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

			mapData["lights"] = nlohmann::json::array();
			activeMapView->map->rootNode->traverse([&](Node* n) {
				auto light = n->getComponent<RswLight>();
				auto object = n->getComponent<RswObject>();
				if (light && object)
				{
					nlohmann::json l;
					to_json(l, *light);
					to_json(l, *object);
					mapData["lights"].push_back(l);
				}
			});

			mapData["floorTiles"] = gnd->getMapQuads();

			mapData["modelPolygons"] = nlohmann::json::array();
			activeMapView->map->rootNode->traverse([&](Node* n) {
				auto rsm = n->getComponent<Rsm>();
				auto rswModel = n->getComponent<RswModel>();
				auto collider = n->getComponent<RswModelCollider>();
				if (rsm && rswModel)
				{
					auto verts = collider->getVerticesWorldSpace();
					for (const auto& v : verts)
						mapData["modelPolygons"].push_back(v);
				}
				});


			std::ofstream mapDataFile("mapdata.json");
			mapDataFile << std::setw(2) << mapData;
		}
		hotkeyMenuItem("Open Model Editor", HotkeyAction::Global_ModelEditor_Open);
		hotkeyMenuItem("Quit", HotkeyAction::Global_Exit);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		hotkeyMenuItem("Undo", HotkeyAction::Global_Undo);
		hotkeyMenuItem("Redo", HotkeyAction::Global_Redo);

		if (editMode == EditMode::Object)
		{
			hotkeyMenuItem("Copy", HotkeyAction::Global_Copy);
			hotkeyMenuItem("Paste", HotkeyAction::Global_Paste);
		}

		if (ImGui::MenuItem("Toggle Undo Window", nullptr, windowData.undoVisible))
			windowData.undoVisible = !windowData.undoVisible;
		hotkeyMenuItem("Global Settings", HotkeyAction::Global_Settings);

		if (ImGui::MenuItem("Hotkey Settings"))
		{
			windowData.hotkeyEditWindowVisible = true;
			windowData.hotkeys = config.hotkeys;
		}			
		if (ImGui::MenuItem("Demo Window", nullptr, windowData.demoWindowVisible))
			windowData.demoWindowVisible = !windowData.demoWindowVisible;
		hotkeyMenuItem("Reload Textures", HotkeyAction::Global_ReloadTextures);
		hotkeyMenuItem("Reload Models", HotkeyAction::Global_ReloadModels);
		ImGui::EndMenu();
	}
	if (editMode == EditMode::Object && activeMapView && ImGui::BeginMenu("Object Selection"))
	{
		if (ImGui::BeginMenu("Grow/shrink Selection"))
		{
			if (ImGui::MenuItem("Select same models"))
				activeMapView->map->selectSameModels(this);
			static float nearDistance = 50;
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat("Near Distance", &nearDistance, 1.0f, 0.0f, 1000.0f);
			if (ImGui::MenuItem("Select objects near"))
				activeMapView->map->selectNear(nearDistance, this);
			if (ImGui::BeginMenu("Select all"))
			{
				hotkeyMenuItem("Select all", HotkeyAction::ObjectEdit_SelectAll);
				hotkeyMenuItem("Select all models", HotkeyAction::ObjectEdit_SelectAllModels);
				hotkeyMenuItem("Select all effects", HotkeyAction::ObjectEdit_SelectAllEffects);
				hotkeyMenuItem("Select all sounds", HotkeyAction::ObjectEdit_SelectAllSounds);
				hotkeyMenuItem("Select all lights", HotkeyAction::ObjectEdit_SelectAllLights);
				ImGui::EndMenu();
			}
			hotkeyMenuItem("Invert selection", HotkeyAction::ObjectEdit_InvertSelection);
			ImGui::EndMenu();
		}
		hotkeyMenuItem("Flip horizontally", HotkeyAction::ObjectEdit_FlipHorizontal);
		hotkeyMenuItem("Flip vertically", HotkeyAction::ObjectEdit_FlipVertical);

		hotkeyMenuItem("Invert Scale X", HotkeyAction::ObjectEdit_InvertScaleX);
		hotkeyMenuItem("Invert Scale Y", HotkeyAction::ObjectEdit_InvertScaleY);
		hotkeyMenuItem("Invert Scale Z", HotkeyAction::ObjectEdit_InvertScaleZ);

		hotkeyMenuItem("Create Prefab", HotkeyAction::ObjectEdit_CreatePrefab);
		hotkeyMenuItem("Delete", HotkeyAction::ObjectEdit_Delete);
		hotkeyMenuItem("Focus on selection", HotkeyAction::ObjectEdit_FocusOnSelection);
		if (ImGui::MenuItem("Set to floor height") && activeMapView)
			activeMapView->map->setSelectedItemsToFloorHeight(this);

		ImGui::EndMenu();
	}
	if (editMode == EditMode::Object && activeMapView && ImGui::BeginMenu("Light Edit"))
	{
		if (ImGui::MenuItem("Add new light")) {
			auto l = new RswLight();
			l->range = 60.0f;
			l->color = glm::vec3(0.8f, 0.8f, 0.8f);
			Node* newNode = new Node("light");
			newNode->addComponent(new RswObject());
			newNode->addComponent(l);
			newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
			newNode->addComponent(new CubeCollider(5));
			newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
			newNodesCenter = glm::vec3(0, -35, 0);
			newNodePlacement = BrowEdit::Relative;
		}

		if (ImGui::MenuItem("Add new sun")) {
			auto l = new RswLight();
			l->range = 0.0f;
			l->intensity = 0.5f;
			l->givesShadow = true;
			l->affectShadowMap = true;
			l->sunMatchRswDirection = true;
			l->diffuseLighting = false;
			l->lightType = RswLight::Type::Sun;
			Node* newNode = new Node("sun");
			newNode->addComponent(new RswObject());
			newNode->addComponent(l);
			newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
			newNode->addComponent(new CubeCollider(5));
			newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
			newNodesCenter = glm::vec3(0, 0, 0);
			newNodePlacement = BrowEdit::Ground;
		}

		if (ImGui::MenuItem("Select all lights")) {
			if (activeMapView)
				activeMapView->map->selectAll<RswLight>(this);
		}

		ImGui::EndMenu();
	}

	if(activeMapView)
	if (ImGui::BeginMenu(activeMapView->map->name.c_str()))
	{
		if (ImGui::MenuItem("Open new view"))
			loadMap(activeMapView->map->name);
		if (ImGui::BeginMenu("Import/Export maps"))
		{
			static bool exportWalls = true;
			static bool exportBorders = true;
			ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
			ImGui::MenuItem("Import/Export Walls", nullptr, &exportWalls);
			ImGui::MenuItem("Import/Export Borders", nullptr, &exportBorders);
			ImGui::PopItemFlag();
			ImGui::Separator();
			if (ImGui::MenuItem("Export shadowmap"))
				activeMapView->map->exportShadowMap(this, exportWalls, exportBorders);
			if (ImGui::MenuItem("Export colormap"))
				activeMapView->map->exportLightMap(this, exportWalls, exportBorders);
			if (ImGui::MenuItem("Export tile colors"))
				activeMapView->map->exportTileColors(this, exportWalls);
			ImGui::Separator();
			if (ImGui::MenuItem("Import shadowmap"))
				activeMapView->map->importShadowMap(this, exportWalls, exportBorders);
			if (ImGui::MenuItem("Import colormap"))
				activeMapView->map->importLightMap(this, exportWalls, exportBorders);
			if (ImGui::MenuItem("Import tile colors"))
				activeMapView->map->importTileColors(this, exportWalls);


			ImGui::EndMenu();
		}
		hotkeyMenuItem("Calculate Quadtree", HotkeyAction::Global_CalculateQuadtree);
		hotkeyMenuItem("Calculate lightmaps", HotkeyAction::Global_CalculateLightmaps);
		if (ImGui::MenuItem("Make tiles unique"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeTilesUnique();
		hotkeyMenuItem("Remove zero height walls", HotkeyAction::Global_ClearZeroHeightWalls);
		if (ImGui::MenuItem("Clean Tiles"))
			activeMapView->map->rootNode->getComponent<Gnd>()->cleanTiles();
		if (ImGui::MenuItem("Edit lighting/water/fog")) {
			editMode = EditMode::Object;
			activeMapView->map->doAction(new SelectAction(activeMapView->map, activeMapView->map->rootNode, false, false), this);
		}
		if (ImGui::MenuItem("Fix lightmap borders"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeLightmapBorders(this);
		if (ImGui::MenuItem("Clear lightmaps"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeLightmapsClear();
		if (ImGui::MenuItem("Smoothen lightmaps"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeLightmapsSmooth(this);
		if (ImGui::MenuItem("Make lightmaps unique"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeLightmapsUnique();
		if (ImGui::MenuItem("Clean up lightmaps"))
			activeMapView->map->rootNode->getComponent<Gnd>()->cleanLightmaps();
		static int s[2] = { activeMapView->map->rootNode->getComponent<Gnd>()->lightmapWidth, activeMapView->map->rootNode->getComponent<Gnd>()->lightmapHeight };
		ImGui::InputInt2("##lightmapRes", s);
		ImGui::SameLine();
		if (ImGui::MenuItem("Change Lightmap Resolution"))
			activeMapView->map->rootNode->getComponent<Gnd>()->makeLightmapsDiffRes(s[0], s[1]);

		if (activeMapView->map->name == "data\\effects_ro.rsw" && ImGui::MenuItem("Generate effects")) //speedrun map
		{
			auto effectsFile = util::FileIO::open("data\\EffectTable.json");
			int braces = 0;
			bool singleQuote = false;
			bool doubleQuote = false;
			int comment = 0;
			int mlComment = 0;
			std::string buffer = "";
			std::set<int> ids;
			while (!effectsFile->eof())
			{
				char c = effectsFile->get();
				if (c == '{' && !singleQuote && !doubleQuote)
					braces++;
				else if (c == '}' && !singleQuote && !doubleQuote)
					braces--;
				else if (c == '[' && !singleQuote && !doubleQuote)
					braces++;
				else if (c == ']' && !singleQuote && !doubleQuote)
					braces--;
				else if (c == '\'')
					singleQuote = !singleQuote;
				else if (c == '\"')
					doubleQuote = !doubleQuote;
				else if (c == ',')
					buffer = "";
				else if (c == '/' && mlComment == 2)
					mlComment = 0;
				else if (c == '/')
					comment++;
				else if (c == '*' && mlComment == 1)
					mlComment = 2;
				else if (c == '*' && comment == 1)
					mlComment = 1;
				else if (c == '\t')
					;
				else if (c == ' ')
					;
				else if (c == '\r')
					;
				else if (c == '\n')
				{
					comment = 0;
					doubleQuote = false;
					singleQuote = false;
				}
				else if (c == ':')
				{
					if (buffer != "")
					{
						try
						{
							int id = std::stoi(buffer);
							if (id != 0)
								ids.insert(id);
						}
						catch (...) {}
						buffer = "";
					}
				}
				else if (braces == 1 && comment == 0 && mlComment == 0)
					buffer += c;
			}
			delete effectsFile;

			activeMapView->map->rootNode->children.clear(); //oops
			auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

			int tU[6][6];
			int tF[6][6];
			int tS[6][6];
			int digits[10];

			int xx = 5 + 6 * (0 % 40) - 1;
			int yy = 17 + 6 * (0 / 40) - 1;
			for (int x = xx; x < xx + 6; x++)
			{
				for (int y = yy; y < yy + 6; y++)
				{
					tU[x - xx][y - yy] = gnd->cubes[x][gnd->height - 1 - y]->tileUp;
					tF[x - xx][y - yy] = gnd->cubes[x][gnd->height - 1 - y]->tileFront;
					tS[x - xx][y - yy] = gnd->cubes[x][gnd->height - 1 - y]->tileSide;
				}
			}
			for (int x = 0; x < 10; x++)
			{
				xx = 5 + 6 * (x % 40) - 1;
				yy = 17 + 6 * (0 / 40) - 1;
				digits[x] = gnd->cubes[xx + 1][gnd->height - 1 - (yy + 5)]->tileUp;
			}

			for (int x = 0; x < gnd->width; x++)
			{
				for (int y = 0; y < gnd->height-15; y++)
				{
					gnd->cubes[x][y]->h1 = -30;
					gnd->cubes[x][y]->h2 = -30;
					gnd->cubes[x][y]->h3 = -30;
					gnd->cubes[x][y]->h4 = -30;
					activeMapView->map->rootNode->getComponent<GndRenderer>()->setChunkDirty(x, y);
				}
			}
			for (int i = 0; i < 1000; i++)
			{
				bool enabled = std::find(ids.begin(), ids.end(), i) != ids.end();

				int xx = 5+  6 * (i % 40)-1;
				int yy = 17+ 6 * (i / 40)-1;
				for (int x = xx; x < xx + 6; x++)
				{
					for (int y = yy; y < yy + 6; y++)
					{
						float h = -40;
						if (x == xx || x == xx + 5 || y == yy || y == yy + 5)
							h = -30;
						gnd->cubes[x][gnd->height - 1 - y]->h1 = h;
						gnd->cubes[x][gnd->height - 1 - y]->h2 = h;
						gnd->cubes[x][gnd->height - 1 - y]->h3 = h;
						gnd->cubes[x][gnd->height - 1 - y]->h4 = h;
						gnd->cubes[x][gnd->height - 1 - y]->tileFront = tF[x-xx][y-yy];
						gnd->cubes[x][gnd->height - 1 - y]->tileSide = tS[x - xx][y - yy];
						gnd->cubes[x][gnd->height - 1 - y]->tileUp = tU[x - xx][y - yy];
					}
				}
				if (enabled)
				{
					int nr = i;
					int len = (int)floor(log10(nr));
					while (nr > 0)
					{
						gnd->cubes[xx + 1 + len][gnd->height - 1 - (yy + 5)]->tileUp = digits[nr % 10];
						nr /= 10;
						len--;
					}

					auto e = new RswEffect();
					auto o = new RswObject();
					o->position.x = -1180.0f + 60 * (i % 40);
					o->position.y = -48.0f;
					o->position.z = 1060.0f - 60 * (i / 40);
					e->id = i;
					Node* newNode = new Node("Effect" + std::to_string(i));
					newNode->addComponent(o);
					newNode->addComponent(e);
					newNode->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
					newNode->addComponent(new CubeCollider(5));
					newNode->setParent(activeMapView->map->rootNode);
				}
			}

		}


		for (auto mv : mapViews)
		{
			if (mv.map == activeMapView->map)
			{
				if (ImGui::BeginMenu(mv.viewName.c_str()))
				{
					if (ImGui::BeginMenu("High Res Screenshot"))
					{
						static int size[2] = { 2048, 2048 };
						ImGui::DragInt2("Size", size);
						if (ImGui::MenuItem("Save"))
						{
							std::string filename = util::SaveAsDialog("screenshot.png", "All\0*.*\0Png\0*.png");
							if (filename != "")
							{
								mv.fbo->resize(size[0], size[1]);
								mv.render(this);
								mv.fbo->saveAsFile(filename);
							}
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
			}
		}
		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Help"))
		windowData.helpWindowVisible = true;

	ImGui::EndMainMenuBar();


}

