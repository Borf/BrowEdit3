#include <Windows.h> //to remove ugly warning :(
#include "BrowEdit.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "Map.h"
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include "NodeRenderer.h"
#include "Node.h"
#include "Gadget.h"
#include <browedit/actions/Action.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/DeleteObjectAction.h>
#include <browedit/actions/GroupAction.h>
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/RsmRenderer.h"
#include "components/ImguiProps.h"
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>


#ifdef _WIN32
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

int main()
{
	std::cout << R"V0G0N(                                  ';cllllllc:,                                  
                                ,lodddddddddddoc,                               
                               cdddddddddddoddddo:                              
                              :dddddddddodxOO0000x:                             
       ;:looooolc;'        '',:oddddddodxOXNNNNNNXd'                            
     :odddddddddddoc' ,;:clooooddddddlcc:o0NNNNNNXo                             
   'ldddddddddddddddlclodddddddddddddooc:lxKXNNNKd'                             
  'lddddddddddddddddooddddddddddddddddddddodxO0k:                               
  :ddddodxxxxxdoodOK0kdddddddddddddddddddddddo:                       ';:'      
  ldddkOKXXXXOddOXWMMWX0kxdddddddddddddddolllol'                 ';coxO0d,      
  cod0XNNNNNNKOkXMMMMMMMWNX0Oxddddddddo;'','  ,:'              ':dk0KKkc        
  ,lOXNNNNNNNNKKWMMMMMMMMMMMMWXK0Okxxd;,cxXKd  ';'          ',;;::o0Ol'         
    l0NNNNNNNX0NMMMMMMWNKKXWMMMMMMWWNXocxKMMXc;odd:      ',;;;:;;;:c,           
     ,lxO000OxOWMMMMW0l,;cooxXMMMMMMMMXd:coo; 'xXN0l  ',;;;:::;;,               
          '' 'xWMMMWk  :KWWKllXMMMMMMMMWN0oc:co0XOdl:;;;;:;;;,'                 
              xMMMMXl  xMMMWd,OMMMMWXXNMMMMMMWXOdc;;;:::;;,'                    
              dWMMMXl;:cokxl ;KMMMMN0KWMMMWXOdl:;;;::::,'                       
              lNMMMWKXX:    cKMMMMMMMMMMMMWOc:c::cloo:                          
              :XWWWWWWXOxxOXWMMMMMMMMN0OOkxl:cc::dOOx,                          
               xXXXXXXXNMMMMMMMWNNWX0dc::::cc::cdOOx;                           
               ,OWMWNXKXXXNNMWNOooolc:::::::cokKWNk,                            
                'lkOKXX0kOOkkxlc::::::;::cokKNMMMWKl'                           
                ;dOKWMNOOOxl;;:::::;;:lxk0NWMMMMMMMW0l                          
               c0NWMMNOool:;;:::;;:cd0NWMMMMMMMMMMMMMWO:                        
           ';:xNMMN0xo:;;;:::;;;cdOXWMMMMMMMMMMMMMMMMMWOdo;                     
          'dOO0X0xo:;;;:::;;;:okKWMMMMMMMMMMMMMMMMMMMWKOKO:                     
           ckxlooc;;::::;;:okKNMMMMMMMMMMMMMMMMMMMMMMXOOx;                      
           'll::c::::;;;:oxO0OOkkkkkkkxxxxddddxxkOOkxl:;                        
         ,:clc::::;;;,   ''                                                     
      ',;c:::::;;;,'                                                            
   ;llc;;:::;;;,'                                                               
  l0Okko:;;;,'                                                                  
  :xlcOk:,'                                  Borf's Browedit  
   ,clo:                                                                        


)V0G0N" << std::endl;
	BrowEdit().run();
}


void BrowEdit::run()
{
	if (!glfwBegin())
	{
		std::cerr << "Error initializing glfw" << std::endl;
		return;
	}
	if (!imguiBegin())
	{
		std::cerr << "Error initializing imgui" << std::endl;
		return;
	}
	configBegin();
	if (config.closeObjectWindowOnAdd)
		windowData.objectWindowVisible = false;

	{
		std::ifstream tagListFile("data\\tags.json");
		if (tagListFile.is_open())
		{
			try {
				json tagListJson;
				tagListFile >> tagListJson;
				tagListFile.close();
				tagList = tagListJson.get<std::map<std::string, std::vector<std::string>>>();
				for (auto tag : tagList)
					for (const auto& t : tag.second)
						tagListReverse[util::utf8_to_iso_8859_1(t)].push_back(util::utf8_to_iso_8859_1(tag.first));
			}
			catch (...) {}
		}
	}

	backgroundTexture = new gl::Texture("data\\background.png", false);
	iconsTexture = new gl::Texture("data\\icons.png", false);
	NodeRenderer::begin();
	Gadget::init();

#ifdef _DEBUG
	if(config.isValid() == "")
//		loadMap("data\\comodo.rsw");
		loadMap("data\\guild_vs1.rsw");
#endif


	while (true)
	{
		if (!glfwLoopBegin())
			break;
		imguiLoopBegin();

		menuBar();
		toolbar();

		if (windowData.configVisible)
			windowData.configVisible = !config.showWindow(this);
		if (windowData.openVisible)
			openWindow();
		if (windowData.undoVisible)
			showUndoWindow();
		if (windowData.objectWindowVisible)
			showObjectWindow();
		if (windowData.demoWindowVisible)
			ImGui::ShowDemoWindow(&windowData.demoWindowVisible);

		if (editMode == EditMode::Object)
		{
			showObjectTree();
			showObjectProperties();
		}


		for (auto it = mapViews.begin(); it != mapViews.end(); )
		{
			showMapWindow(*it);
			if (!it->opened)
			{
				if (activeMapView == &(*it))
					activeMapView = nullptr;
				Map* map = it->map;
				it = mapViews.erase(it);
			}
			else
				it++;
		}

		if (!ImGui::GetIO().WantTextInput)
		{
			if (ImGui::GetIO().KeyCtrl)
			{
				if (ImGui::IsKeyPressed('Z') && activeMapView)
					if (ImGui::GetIO().KeyShift)
						activeMapView->map->redo(this);
					else
						activeMapView->map->undo(this);
				if (ImGui::IsKeyPressed('Y') && activeMapView)
					activeMapView->map->redo(this);
				if (ImGui::IsKeyPressed('S') && activeMapView)
					saveMap(activeMapView->map);
			}
			if (editMode == EditMode::Object && activeMapView)
			{
				if (ImGui::IsKeyPressed(GLFW_KEY_DELETE) && activeMapView && activeMapView->map->selectedNodes.size() > 0)
				{
					auto ga = new GroupAction();
					for (auto n : activeMapView->map->selectedNodes)
						ga->addAction(new DeleteObjectAction(n));
					activeMapView->map->doAction(ga, this);
				}
				if (ImGui::IsKeyPressed('C'))
				{
					json clipboard;
					for (auto n : activeMapView->map->selectedNodes)
						clipboard.push_back(*n);
					ImGui::SetClipboardText(clipboard.dump(1).c_str());
				}

				if(ImGui::IsKeyPressed('V'))
				{
					try
					{
						json clipboard = json::parse(ImGui::GetClipboardText());
						if (clipboard.size() > 0)
						{
							for (auto n : newNodes) //TODO: should I do this?
								delete n.first;
							newNodes.clear();
/*							glm::vec3 center(0, 0, 0);
							for (auto n : activeMapView->map->selectedNodes)
								center += n->getComponent<RswObject>()->position;
							center /= activeMapView->map->selectedNodes.size();*/

							for (auto n : clipboard)
							{
								auto newNode = new Node(n["name"].get<std::string>());
								for (auto c : n["components"])
								{
									if (c["type"] == "rswobject")
									{
										auto rswObject = new RswObject();
										from_json(c, *rswObject);
										newNode->addComponent(rswObject);
									}
									if (c["type"] == "rswmodel")
									{
										auto rswModel = new RswModel();
										from_json(c, *rswModel);
										newNode->addComponent(rswModel);
										newNode->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + util::utf8_to_iso_8859_1(rswModel->fileName)));
										newNode->addComponent(new RsmRenderer());
										newNode->addComponent(new RsmRenderer());
										newNode->addComponent(new RswModelCollider());
									}
								}
								newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, newNode->getComponent<RswObject>()->position));
							}
							glm::vec3 center(0, 0, 0);
							for (auto& n : newNodes)
								center += n.second;
							center /= newNodes.size();

							for (auto& n : newNodes)
								n.second = n.second - center;
						}
					}
					catch (...) {};
				}
			}
		}


		imguiLoopEnd();
		glfwLoopEnd();
	}
	
	NodeRenderer::end();
	imguiEnd();
	glfwEnd();
}

void BrowEdit::configBegin()
{
	json configJson;
	std::ifstream configFile("config.json");
	if (configFile.is_open())
	{
		configFile >> configJson;
		configFile.close();
		try {
			config = configJson.get<Config>();
		}
		catch (...) {}
		if (config.isValid() != "")
		{
			windowData.configVisible = true;
			util::FileIO::begin();
			util::FileIO::addDirectory(".\\");
			util::FileIO::end();
		}
		else
			config.setupFileIO();
	}
	else
	{
		windowData.configVisible = true;
		util::FileIO::begin();
		util::FileIO::addDirectory(".\\");
		util::FileIO::end();
	}
}

bool BrowEdit::toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip)
{
	ImVec2 v1((1.0f / iconsTexture->width) * (36 * (icon%4) + 1.5f), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (36 * (icon/4) + 1.5f));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 34, v1.y + (1.0f / iconsTexture->height) * 34);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(32, 32), v1, v2, 0, ImVec4(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, *status ? 1.0f : 0.0f));
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	if(clicked)
		*status = !*status;
	ImGui::PopID();
	return clicked;
}

bool BrowEdit::toolBarToggleButton(const std::string_view& name, int icon, bool status, const char* tooltip)
{
	ImVec2 v1((1.0f / iconsTexture->width) * (36 * (icon % 4) + 1.5f), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (36 * (icon / 4) + 1.5f));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 34, v1.y + (1.0f / iconsTexture->height) * 34);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(32, 32), v1, v2, 0, ImVec4(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, status ? 1.0f : 0.0f));
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	ImGui::PopID();
	return clicked;
}
void BrowEdit::showMapWindow(MapView& mapView)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(2048, 2048));
	if (ImGui::Begin(mapView.viewName.c_str(), &mapView.opened))
	{
		toolBarToggleButton("ortho", 11, &mapView.ortho, "Toggle between ortho and perspective camera");
		ImGui::SameLine();

		toolBarToggleButton("viewLightMapShadow", 0, &mapView.viewLightmapShadow, "Toggle shadowmap");
		ImGui::SameLine();
		toolBarToggleButton("viewLightmapColor", 1, &mapView.viewLightmapColor, "Toggle colormap");
		ImGui::SameLine();
		toolBarToggleButton("viewColors", 2, &mapView.viewColors, "Toggle tile colors");
		ImGui::SameLine();
		toolBarToggleButton("viewLighting", 3, &mapView.viewLighting, "Toggle lighting");
		ImGui::SameLine();
		toolBarToggleButton("smoothColors", 2, &mapView.smoothColors, "Smooth colormap");
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();

		bool snapping = mapView.snapToGrid;
		if (ImGui::GetIO().KeyShift)
			snapping = !snapping;
		bool ret = toolBarToggleButton("snapToGrid", 7, snapping, "Snap to grid");
		if (!ImGui::GetIO().KeyShift && ret)
			mapView.snapToGrid = !mapView.snapToGrid;
		if (snapping || mapView.snapToGrid)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(50);
			ImGui::DragFloat("##gridSize", &mapView.gridSize, 1.0f, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Grid size. Doubleclick or ctrl+click to type a number");
			ImGui::SameLine();
			ImGui::Checkbox("##gridLocal", &mapView.gridLocal);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Local or Global grid. Either makes the movement rounded off, or the final position");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();

		if (BrowEdit::editMode == EditMode::Object)
		{
			if (toolBarToggleButton("translate", 8, mapView.gadget.mode == Gadget::Mode::Translate, "Move"))
				mapView.gadget.mode = Gadget::Mode::Translate;
			ImGui::SameLine();
			if (toolBarToggleButton("rotate", 9, mapView.gadget.mode == Gadget::Mode::Rotate, "Rotate"))
				mapView.gadget.mode = Gadget::Mode::Rotate;
			ImGui::SameLine();
			if (toolBarToggleButton("scale", 10, mapView.gadget.mode == Gadget::Mode::Scale, "Scale"))
				mapView.gadget.mode = Gadget::Mode::Scale;


			if (mapView.gadget.mode == Gadget::Mode::Rotate || mapView.gadget.mode == Gadget::Mode::Scale)
			{
				ImGui::SameLine();
				if (toolBarToggleButton("localglobal", mapView.pivotPoint == MapView::PivotPoint::Local ? 12 : 13, false, "Changes the pivot point for rotations"))
				{
					if (mapView.pivotPoint == MapView::PivotPoint::Local)
						mapView.pivotPoint = MapView::PivotPoint::GroupCenter;
					else
						mapView.pivotPoint = MapView::PivotPoint::Local;
				}

			}
		}


		auto size = ImGui::GetContentRegionAvail();
		if (mapView.map->rootNode->getComponent<Gnd>())
		{
			mapView.update(this, size);
			mapView.render(this);
			if (editMode == EditMode::Object)
				mapView.postRenderObjectMode(this);
			mapView.prevMouseState = mapView.mouseState; //meh
		}

		ImTextureID id = (ImTextureID)((long long)mapView.fbo->texid[0]); //TODO: remove cast for 32bit
		ImGui::Image(id, size, ImVec2(0,1), ImVec2(1,0));
		mapView.hovered = ImGui::IsItemHovered();
	}
	if (ImGui::IsWindowFocused())
	{
		if (activeMapView != &mapView)
			std::cout << "Switching map to " << mapView.viewName<<std::endl;
		activeMapView = &mapView;
	}
	ImGui::End();

}



void BrowEdit::menuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New", "Ctrl+n");
		if (ImGui::MenuItem("Open", "Ctrl+o"))
		{
			windowData.openFiles = util::FileIO::listFiles("data");
			windowData.openFiles.erase(std::remove_if(windowData.openFiles.begin(), windowData.openFiles.end(), [](const std::string& map) { return map.substr(map.size()-4, 4) != ".rsw"; }), windowData.openFiles.end());
			windowData.openVisible = true;
		}
		if (activeMapView && ImGui::MenuItem(("Save " + activeMapView->map->name).c_str(), "Ctrl+s"))
		{
			saveMap(activeMapView->map);
		}
		if(ImGui::MenuItem("Quit"))
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
		ImGui::EndMenu();
	}
	if (editMode == EditMode::Object && activeMapView && ImGui::BeginMenu("Selection"))
	{
		if (ImGui::BeginMenu("Grow/shrink Selection"))
		{
			if (ImGui::MenuItem("Select same models"))
			{
				std::set<std::string> files;
				for(auto n : activeMapView->map->selectedNodes)
				{
					auto rswModel = n->getComponent<RswModel>();
					if (rswModel)
						files.insert(rswModel->fileName);
				}
				auto ga = new GroupAction();
				activeMapView->map->rootNode->traverse([&](Node* n) {
					if (std::find(activeMapView->map->selectedNodes.begin(), activeMapView->map->selectedNodes.end(), n) != activeMapView->map->selectedNodes.end())
						return;
					auto rswModel = n->getComponent<RswModel>();
					if (rswModel && std::find(files.begin(), files.end(), rswModel->fileName) != files.end())
					{
						auto sa = new SelectAction(activeMapView->map, n, true, false);
						sa->perform(activeMapView->map, this);
						ga->addAction(sa);
					}
				});
				activeMapView->map->doAction(ga, this);
			}
			static float nearDistance = 50;
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat("Near Distance", &nearDistance, 1.0f, 0.0f, 1000.0f);
			if (ImGui::MenuItem("Select objects near"))
			{
				auto selection = activeMapView->map->selectedNodes;
				auto ga = new GroupAction();
				activeMapView->map->rootNode->traverse([&](Node* n) {
					auto rswModel = n->getComponent<RswModel>();
					auto rswObject = n->getComponent<RswObject>();
					if (!rswObject)
						return;
					auto distance = 999999999;
					for (auto nn : selection)
						if (glm::distance(nn->getComponent<RswObject>()->position, rswObject->position) < distance)
							distance = glm::distance(nn->getComponent<RswObject>()->position, rswObject->position);
					if (distance < nearDistance)
					{
						auto sa = new SelectAction(activeMapView->map, n, true, false);
						sa->perform(activeMapView->map, this);
						ga->addAction(sa);
					}
					});
				activeMapView->map->doAction(ga, this);

			}
			if (ImGui::MenuItem("Select all"))
			{
				auto ga = new GroupAction();
				activeMapView->map->rootNode->traverse([&](Node* n) {
					if (std::find(activeMapView->map->selectedNodes.begin(), activeMapView->map->selectedNodes.end(), n) != activeMapView->map->selectedNodes.end())
						return;
					auto rswModel = n->getComponent<RswModel>();
					if (rswModel)
					{
						auto sa = new SelectAction(activeMapView->map, n, true, false);
						sa->perform(activeMapView->map, this);
						ga->addAction(sa);
					}
					});
				activeMapView->map->doAction(ga, this);
			}
			if (ImGui::MenuItem("Invert selection"))
			{
				auto ga = new GroupAction();
				activeMapView->map->rootNode->traverse([&](Node* n) {
					bool selected = std::find(activeMapView->map->selectedNodes.begin(), activeMapView->map->selectedNodes.end(), n) != activeMapView->map->selectedNodes.end();

					auto rswModel = n->getComponent<RswModel>();
					if (rswModel)
					{
						auto sa = new SelectAction(activeMapView->map, n, true, selected);
						sa->perform(activeMapView->map, this);
						ga->addAction(sa);
					}
					});
				activeMapView->map->doAction(ga, this);
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Flip horizontally"))
		{

		}
		if (ImGui::MenuItem("Flip vertically"))
		{

		}
		if (ImGui::MenuItem("Delete", "Delete"))
		{

		}
		if (ImGui::MenuItem("Go to selection", "F"))
		{

		}
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
	ImGui::EndMainMenuBar();
}


void BrowEdit::toolbar()
{
	auto viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));
	ImGuiWindowFlags toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Toolbar", 0, toolbarFlags);

	if(editMode == EditMode::Texture)
		ImGui::Text("Texture");
	else if (editMode == EditMode::Object)
		ImGui::Text("Object");
	else if (editMode == EditMode::Wall)
		ImGui::Text("Wall");
	ImGui::SameLine();

	if (toolBarToggleButton("texturemode", 4, editMode == EditMode::Texture, "Texture edit mode"))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("objectmode", 5, editMode == EditMode::Object, "Object edit mode"))
		editMode = EditMode::Object;
	ImGui::SameLine();
	if (toolBarToggleButton("wallmode", 6, editMode == EditMode::Wall, "Wall edit mode"))
		editMode = EditMode::Wall;
	ImGui::SameLine();

	if(editMode == EditMode::Object)
		toolBarToggleButton("showObjectWindow", 5, &windowData.objectWindowVisible, "Toggle object window");

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - toolbarHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));
	toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Statusbar", 0, toolbarFlags);
	ImGui::Text("Browedit!");
	ImGui::SameLine();
	ImGui::End();
}



void BrowEdit::openWindow()
{
	ImGui::OpenPopup("Open Map");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Open Map", 0, ImGuiWindowFlags_NoDocking))
	{
		ImGui::Button("Browse for file");

		ImGui::Text("Filter");
		ImGui::InputText("##filter", &windowData.openFilter);

		if (ImGui::BeginListBox("##Files"))
		{
			for (std::size_t i = 0; i < windowData.openFiles.size(); i++)
			{
				if(windowData.openFilter == "" || windowData.openFiles[i].find(windowData.openFilter) != std::string::npos)
					if (ImGui::Selectable(windowData.openFiles[i].c_str(), windowData.openFileSelected == i))
						windowData.openFileSelected = i;
			}
			ImGui::EndListBox();
		}

		if (ImGui::Button("Open"))
		{
			loadMap(windowData.openFiles[windowData.openFileSelected]);
			windowData.openVisible = false;
		}
		ImGui::EndPopup();
	}
}


void BrowEdit::saveMap(Map* map)
{
	map->rootNode->getComponent<Rsw>()->save("out\\" + map->name);
}

void BrowEdit::loadMap(const std::string& file)
{
	Map* map = nullptr;
	for (auto m : maps)
		if (m->name == file)
			map = m;
	if (!map)
	{
		map = new Map(file);
		maps.push_back(map);
	}
	int viewCount = 0;
	for (const auto& mv : mapViews)
		if (mv.map == map && mv.viewName == file + "#" + std::to_string(viewCount))
			viewCount++;
	mapViews.push_back(MapView(map, file + "#" + std::to_string(viewCount)));
}



void BrowEdit::showObjectTree()
{
	ImGui::Begin("Objects");
	ImGui::PushFont(font);
	for (auto m : maps)
	{
		buildObjectTree(m->rootNode, m);
	}
	ImGui::PopFont();

	ImGui::End();
}


void BrowEdit::buildObjectTree(Node* node, Map* map)
{
	if (node->children.size() > 0)
	{
		int flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (std::find(map->selectedNodes.begin(), map->selectedNodes.end(), node) != map->selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
		if (ImGui::IsItemClicked())
		{
			map->doAction(new SelectAction(map, node, false, false), this);
		}
		if(opened)
		{
			for (auto n : node->children)
				buildObjectTree(n, map);
			ImGui::TreePop();
		}
	}
	else
	{
		int flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (std::find(map->selectedNodes.begin(), map->selectedNodes.end(), node) != map->selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;


		ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(textColor.x, textColor.y, textColor.z, h,s,v);
		if (node->getComponent<RswModel>())		{	h = 0;		s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f);}
		if (node->getComponent<RswEffect>())	{	h = 0.25f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f);}
		if (node->getComponent<RswSound>())		{	h = 0.5f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f);}
		if (node->getComponent<RswLight>())		{	h = 0.75f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f);}
		ImGui::ColorConvertHSVtoRGB(h, s, v, textColor.x, textColor.y, textColor.z);

		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
		ImGui::PopStyleColor();

		if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
		{
			auto rswObject = node->getComponent<RswObject>();
			if (rswObject)
			{
				auto gnd = map->rootNode->getComponent<Gnd>();
				for (auto& m : mapViews)
				{
					if (m.map->rootNode == node->root)
					{
						m.cameraCenter.x = 5 * gnd->width + rswObject->position.x;
						m.cameraCenter.y = -1 * (-10 - 5 * gnd->height + rswObject->position.z);
					}
				}
			}
		}
		if (ImGui::IsItemClicked())
		{
			map->doAction(new SelectAction(map, node, false, false), this);
		}
	}
}


void BrowEdit::showObjectProperties()
{
	ImGui::Begin("Properties");
	ImGui::PushFont(font);
	if (activeMapView && activeMapView->map->selectedNodes.size() == 1)
	{
		if (util::InputText(this, activeMapView->map, activeMapView->map->selectedNodes[0], "Name", &activeMapView->map->selectedNodes[0]->name, 0, "Renaming"))
		{
			activeMapView->map->selectedNodes[0]->onRename();
		}
		for (auto c : activeMapView->map->selectedNodes[0]->components)
		{
			auto props = dynamic_cast<ImguiProps*>(c);
			if (props)
				props->buildImGui(this);
		}
	}
	ImGui::PopFont();
	ImGui::End();
}


void BrowEdit::showUndoWindow()
{
	ImGui::Begin("Undo stack", &windowData.undoVisible);
	if (activeMapView && ImGui::BeginListBox("##stack", ImGui::GetContentRegionAvail()))
	{
		for (auto action : activeMapView->map->undoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		for (auto action : activeMapView->map->redoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PopStyleColor();
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		ImGui::EndListBox();
	}
	ImGui::End();
}


class ObjectWindowObject
{
public:
	Node* node;
	gl::FBO* fbo;
	NodeRenderContext nodeRenderContext;
	float rotation = 0;
	BrowEdit* browEdit;

	ObjectWindowObject(const std::string &fileName, BrowEdit* browEdit) : browEdit(browEdit)
	{
		fbo = new gl::FBO((int)browEdit->config.thumbnailSize.x, (int)browEdit->config.thumbnailSize.y, true); //TODO: resolution?
		node = new Node();
		node->addComponent(util::ResourceManager<Rsm>::load(fileName));
		node->addComponent(new RsmRenderer());
	}

	void draw()
	{
		auto rsm = node->getComponent<Rsm>();
		if (!rsm->loaded)
			return;
		fbo->bind();
		glViewport(0, 0, fbo->getWidth(), fbo->getHeight());
		glClearColor(browEdit->config.backgroundColor.r, browEdit->config.backgroundColor.g, browEdit->config.backgroundColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float distance = 1.1f* glm::max(glm::abs(rsm->realbbmax.y), glm::max(glm::abs(rsm->realbbmax.z), glm::abs(rsm->realbbmax.x)));

		float ratio = fbo->getWidth() / (float)fbo->getHeight();
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 5000.0f);
		nodeRenderContext.viewMatrix = glm::lookAt(glm::vec3(0.0f, -distance, -distance), glm::vec3(0.0f, rsm->bbrange.y, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
		node->getComponent<RsmRenderer>()->matrixCache = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 1, 0));
		NodeRenderer::render(node, nodeRenderContext);
		fbo->unbind();
	}
};
std::map<std::string, ObjectWindowObject*> objectWindowObjects;

void BrowEdit::showObjectWindow()
{
	ImGui::Begin("Object Picker", &windowData.objectWindowVisible);
	static std::string filter;
	ImGui::InputText("Filter", &filter);
	ImGui::SameLine();
	if (ImGui::Button("Rescan map filters"))
	{
		tagListReverse.clear();
		//remove old map: tags
		for (auto it = tagList.begin(); it != tagList.end(); )
			if (it->first.size() > 4 && it->first.substr(0, 4) == "map:")
				it = tagList.erase(it);
			else
				it++;

		std::vector<std::string> maps = util::FileIO::listFiles("data");
		maps.erase(std::remove_if(maps.begin(), maps.end(), [](const std::string& map) { return map.substr(map.size() - 4, 4) != ".rsw"; }), maps.end());
		for (const auto& fileName : maps)
		{
			Node* node = new Node();
			Rsw* rsw = new Rsw();
			node->addComponent(rsw);
			rsw->load(fileName, false, false);

			std::string mapTag = fileName;
			if (mapTag.substr(0, 5) == "data\\")
				mapTag = mapTag.substr(5);
			if (mapTag.substr(mapTag.size() - 4, 4) == ".rsw")
				mapTag = mapTag.substr(0, mapTag.size() - 4);

			mapTag = "map:" + util::iso_8859_1_to_utf8(mapTag);

			node->traverse([&](Node* node)
			{
				auto rswModel = node->getComponent<RswModel>();
				if (rswModel)
				{
					if(std::find(tagList[mapTag].begin(), tagList[mapTag].end(), rswModel->fileName) == tagList[mapTag].end())
						tagList[mapTag].push_back(rswModel->fileName);
				}
			});
			delete node;
		}
		for (auto tag : tagList)
			for (const auto& t : tag.second)
				tagListReverse[util::utf8_to_iso_8859_1(t)].push_back(util::utf8_to_iso_8859_1(tag.first));

		saveTagList();

	}

	static util::FileIO::Node* selectedNode = nullptr;
	std::function<void(util::FileIO::Node*)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node)
	{
		for (auto f : node->directories)
		{
			int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (f.second->directories.size() == 0)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (f.second == selectedNode)
				flags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(f.second->name.c_str(), flags))
			{
				buildTreeNodes(f.second);
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked())
				selectedNode = f.second;
		}
	};

	ImGui::BeginChild("left pane", ImVec2(250, 0), true);
	if (ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
	{
		auto root = util::FileIO::directoryNode("data\\model\\");
		if(root)
			buildTreeNodes(root);
		ImGui::TreePop();
	}
	ImGui::EndChild();
	ImGui::SameLine();

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;


	auto buildBox = [&](const std::string& file) {
		if (ImGui::BeginChild(file.c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			std::string path = util::utf8_to_iso_8859_1(file);
			auto n = selectedNode;
			while (n)
			{
				if (n->name != "")
					path = util::utf8_to_iso_8859_1(n->name) + "\\" + path;
				n = n->parent;
			}

			auto it = objectWindowObjects.find(path);
			if (it == objectWindowObjects.end())
			{
				objectWindowObjects[path] = new ObjectWindowObject(path, this);
				it = objectWindowObjects.find(path);
				it->second->draw();
			}
			if (ImGui::ImageButton((ImTextureID)(long long)it->second->fbo->texid[0], config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				if (activeMapView && newNodes.size() == 0)
				{
					Node* newNode = new Node(file);
					newNode->addComponent(util::ResourceManager<Rsm>::load(path));
					newNode->addComponent(new RsmRenderer());
					newNode->addComponent(new RswObject());
					newNode->addComponent(new RswModel(path));
					newNode->addComponent(new RsmRenderer());
					newNode->addComponent(new RswModelCollider());
					newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
				}
				std::cout << "Click on " << file << std::endl;
			}
			if (ImGui::BeginPopupContextWindow("Object Tags"))
			{
				static std::string newTag;
				ImGui::SetNextItemWidth(100);
				ImGui::InputText("Tag", &newTag);
				ImGui::SameLine();
				if (ImGui::Button("Add"))
				{
					tagList[newTag].push_back(util::iso_8859_1_to_utf8(path.substr(11))); //remove data\model\ prefix
					tagListReverse[path.substr(11)].push_back(newTag);
					saveTagList();
				}
				for (auto tag : tagListReverse[path.substr(11)])
				{
					ImGui::Text(tag.c_str());
					ImGui::SameLine();
					if (ImGui::Button("Remove"))
					{
						tagList[tag].erase(std::remove_if(tagList[tag].begin(), tagList[tag].end(), [&](const std::string& m) { return m == util::iso_8859_1_to_utf8(path.substr(11)); }), tagList[tag].end());
						tagListReverse[path.substr(11)].erase(std::remove_if(tagListReverse[path.substr(11)].begin(), tagListReverse[path.substr(11)].end(), [&](const std::string& t) { return t == tag; }), tagListReverse[path.substr(11)].end());
						saveTagList();
					}
					
				}

				ImGui::EndPopup();
			}
			else if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(util::combine(tagListReverse[path.substr(11)], "\n").c_str());
				it->second->rotation++;
				it->second->draw();
			}
			ImGui::Text(file.c_str());
		}
		ImGui::EndChild();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.thumbnailSize.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
	};

	if (ImGui::BeginChild("right pane", ImVec2(ImGui::GetContentRegionAvail().x, 0), true))
	{
		if (selectedNode != nullptr && filter == "")
		{
			window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (auto file : selectedNode->files)
			{
				if (file.find(".rsm") == std::string::npos || file.find(".rsm2") != std::string::npos)
					continue;
				buildBox(file);
			}
		}
		else if (filter != "")
		{
			static std::string currentFilterText = "";
			static std::vector<std::string> filteredFiles;
			if (currentFilterText != filter)
			{
				currentFilterText = filter;
				std::vector<std::string> filterTags = util::split(filter, " ");
				filteredFiles.clear();

				for (auto t : tagListReverse)
				{
					bool match = true;
					for (auto tag : filterTags)
					{
						bool tagOk = false;
						for (auto fileTag : t.second)
							if (fileTag.find(tag) != std::string::npos)
							{
								tagOk = true;
								break;
							}
						if (!tagOk)
						{
							match = false;
							break;
						}
					}
					if (match)
						filteredFiles.push_back("data\\model\\" + util::iso_8859_1_to_utf8(t.first));
				}
			}
			for (const auto& file : filteredFiles)
			{
				buildBox(file);
			}

		}
	}
	ImGui::EndChild();
	ImGui::End();
}



void BrowEdit::saveTagList()
{
	json tagListJson = tagList;
	std::ofstream tagListFile("data\\tags.json");
	tagListFile << std::setw(2) << tagListJson;
}