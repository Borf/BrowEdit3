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
#include <GLFW/glfw3.h>
#include <imgui_internal.h>

void BrowEdit::menuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New", "Ctrl+n");
		if (ImGui::MenuItem("Open", "Ctrl+o"))
			showOpenWindow();
		if (activeMapView && ImGui::MenuItem(("Save " + activeMapView->map->name).c_str(), "Ctrl+s"))
			saveMap(activeMapView->map);
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
		if (ImGui::MenuItem("Quit"))
			glfwSetWindowShouldClose(window, 1);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+z") && activeMapView)
			activeMapView->map->undo(this);
		if (ImGui::MenuItem("Redo", "Ctrl+shift+z") && activeMapView)
			activeMapView->map->redo(this);
		if (ImGui::MenuItem("Undo Window", nullptr, windowData.undoVisible))
			windowData.undoVisible = !windowData.undoVisible;
		if (ImGui::MenuItem("Configure"))
			windowData.configVisible = true;
		if (ImGui::MenuItem("Demo Window", nullptr, windowData.demoWindowVisible))
			windowData.demoWindowVisible = !windowData.demoWindowVisible;
		if (ImGui::MenuItem("Reload Textures", "Ctrl+R"))
			for (auto t : util::ResourceManager<gl::Texture>::getAll())
				t->reload();
		if (ImGui::MenuItem("Reload Models", "Shift+R"))
		{
			for (auto rsm : util::ResourceManager<Rsm>::getAll())
				rsm->reload();
			for (auto m : maps)
				m->rootNode->traverse([](Node* n) {
					auto rsmRenderer = n->getComponent<RsmRenderer>();
					if(rsmRenderer)
						rsmRenderer->begin();
				});
		}
		ImGui::EndMenu();
	}
	if (editMode == EditMode::Object && activeMapView && ImGui::BeginMenu("Selection"))
	{
		if (ImGui::MenuItem("Copy", "Ctrl+c"))
			activeMapView->map->copySelection();
		if (ImGui::MenuItem("Paste", "Ctrl+v"))
			activeMapView->map->pasteSelection(this);
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
				if (ImGui::MenuItem("Select all"))
					activeMapView->map->selectAll(this);
				if (ImGui::MenuItem("Select all models"))
					activeMapView->map->selectAll<RswModel>(this);
				if (ImGui::MenuItem("Select all effects"))
					activeMapView->map->selectAll<RswEffect>(this);
				if (ImGui::MenuItem("Select all sounds"))
					activeMapView->map->selectAll<RswSound>(this);
				if (ImGui::MenuItem("Select all lights"))
					activeMapView->map->selectAll<RswLight>(this);
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Invert selection"))
				activeMapView->map->selectInvert(this);
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Flip horizontally"))
			activeMapView->map->flipSelection(0, this);
		if (ImGui::MenuItem("Flip vertically"))
			activeMapView->map->flipSelection(2, this);
		if (ImGui::MenuItem("Delete", "Delete"))
			activeMapView->map->deleteSelection(this);
		if (ImGui::MenuItem("Go to selection", "F"))
			activeMapView->focusSelection();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Maps"))
	{
		for (auto map : maps)
		{
			if (ImGui::BeginMenu(map->name.c_str()))
			{
				if (ImGui::MenuItem("Save as"))
					saveMap(map);
				if (ImGui::MenuItem("Open new view"))
					loadMap(map->name);
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
						map->exportShadowMap(this, exportWalls, exportBorders);
					if (ImGui::MenuItem("Export colormap"))
						map->exportLightMap(this, exportWalls, exportBorders);
					if (ImGui::MenuItem("Export tile colors"))
						map->exportTileColors(this, exportWalls);
					ImGui::Separator();
					if (ImGui::MenuItem("Import shadowmap"))
						map->importShadowMap(this, exportWalls, exportBorders);
					if (ImGui::MenuItem("Import colormap"))
						map->importLightMap(this, exportWalls, exportBorders);
					if (ImGui::MenuItem("Import tile colors"))
						map->importTileColors(this, exportWalls);


					ImGui::EndMenu();
				}
				if(ImGui::MenuItem("Calculate Quadtree", "Ctrl+Q"))
					map->recalculateQuadTree(this);
				if (ImGui::MenuItem("Calculate lightmaps", "Ctrl+L") && !lightmapper)
				{
					lightmapper = new Lightmapper(map, this);
					windowData.openLightmapSettings = true;
				}
				if (ImGui::MenuItem("Make tiles unique"))
					map->rootNode->getComponent<Gnd>()->makeTilesUnique();
				if (ImGui::MenuItem("Clean Tiles"))
					map->rootNode->getComponent<Gnd>()->cleanTiles();
				if (ImGui::MenuItem("Fix lightmap borders"))
					map->rootNode->getComponent<Gnd>()->makeLightmapBorders();
				if (ImGui::MenuItem("Clear lightmaps"))
					map->rootNode->getComponent<Gnd>()->makeLightmapsClear();
				if (ImGui::MenuItem("Smoothen lightmaps"))
					map->rootNode->getComponent<Gnd>()->makeLightmapsSmooth();
				if (ImGui::MenuItem("Make lightmaps unique"))
					map->rootNode->getComponent<Gnd>()->makeLightmapsUnique();

				if (map->name == "data\\effects_ro.rsw" && ImGui::MenuItem("Generate effects")) //speedrun map
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

					map->rootNode->children.clear(); //oops
					auto gnd = map->rootNode->getComponent<Gnd>();

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
							map->rootNode->getComponent<GndRenderer>()->setChunkDirty(x, y);
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
							newNode->setParent(map->rootNode);
						}
					}

				}


				for (auto mv : mapViews)
				{
					if (mv.map == map)
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
		}
		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Help"))
		windowData.helpWindowVisible = true;

	ImGui::EndMainMenuBar();
}

