#include <Windows.h> //to remove ugly warning :(
#include "BrowEdit.h"
#include <browedit/util/FileIO.h>
#include <iostream>
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "Map.h"
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <GLFW/glfw3.h>
#include "NodeRenderer.h"
#include "Node.h"
#include "Gadget.h"
#include <browedit/actions/Action.h>
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/RsmRenderer.h"
#include "components/ImguiProps.h"

#include "actions/SelectAction.h"

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


	backgroundTexture = new gl::Texture("data\\background.png", false);
	iconsTexture = new gl::Texture("data\\icons.png", false);
	NodeRenderer::begin();
	Gadget::init();

#ifdef _DEBUG
	if(config.isValid() == "")
//		loadMap("data\\comodo.rsw");
		loadMap("data\\prontera.rsw");
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

		if (editMode == EditMode::Object)
		{
			showObjectTree();
			showObjectProperties();
		}

		for (std::size_t i = 0; i < mapViews.size(); i++)
		{
			showMapWindow(mapViews[i]);
			if (!mapViews[i].opened)
			{
				Map* map = mapViews[i].map;
				mapViews.erase(mapViews.begin() + i);
				i--;
				//TODO: check if there are still mapviews for map
			}
		}


		if (!ImGui::GetIO().WantTextInput)
		{
			if (ImGui::GetIO().KeyCtrl)
			{
				if (ImGui::IsKeyPressed('Z'))
					if (ImGui::GetIO().KeyShift)
						redo();
					else
						undo();
				if (ImGui::IsKeyPressed('Y'))
					redo();


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

void BrowEdit::doAction(Action* action)
{
	action->perform(nullptr, this);
	undoStack.push_back(action);

	for (auto a : redoStack)
		delete a;
	redoStack.clear();
}

void BrowEdit::redo()
{
	if (redoStack.size() > 0)
	{
		redoStack.front()->perform(nullptr, this);
		undoStack.push_back(redoStack.front());
		redoStack.erase(redoStack.begin());
	}
}
void BrowEdit::undo()
{
	if (undoStack.size() > 0)
	{
		undoStack.back()->undo(nullptr, this);
		redoStack.insert(redoStack.begin(), undoStack.back());
		undoStack.pop_back();
	}
}

bool BrowEdit::toolBarToggleButton(const std::string_view &name, int icon, bool* status)
{
	ImVec2 v1((1.0f / iconsTexture->width) * (36 * (icon%4) + 1.5f), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (36 * (icon/4) + 1.5f));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 34, v1.y + (1.0f / iconsTexture->height) * 34);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(32, 32), v1, v2, 0, ImVec4(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, *status ? 1.0f : 0.0f));
	if(clicked)
		*status = !*status;
	ImGui::PopID();
	return clicked;
}

bool BrowEdit::toolBarToggleButton(const std::string_view& name, int icon, bool status)
{
	ImVec2 v1((1.0f / iconsTexture->width) * (36 * (icon % 4) + 1.5f), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (36 * (icon / 4) + 1.5f));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 34, v1.y + (1.0f / iconsTexture->height) * 34);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(32, 32), v1, v2, 0, ImVec4(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, status ? 1.0f : 0.0f));
	ImGui::PopID();
	return clicked;
}
void BrowEdit::showMapWindow(MapView& mapView)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(2048, 2048));
	if (ImGui::Begin(mapView.viewName.c_str(), &mapView.opened))
	{
		toolBarToggleButton("ortho", 11, &mapView.ortho);
		ImGui::SameLine();

		toolBarToggleButton("viewLightMapShadow", 0, &mapView.viewLightmapShadow);
		ImGui::SameLine();
		toolBarToggleButton("viewLightmapColor", 1, &mapView.viewLightmapColor);
		ImGui::SameLine();
		toolBarToggleButton("viewColors", 2, &mapView.viewColors);
		ImGui::SameLine();
		toolBarToggleButton("viewLighting", 3, &mapView.viewLighting);
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();

		bool snapping = mapView.snapToGrid;
		if (ImGui::GetIO().KeyShift)
			snapping = !snapping;
		bool ret = toolBarToggleButton("snapToGrid", 7, snapping);
		if (!ImGui::GetIO().KeyShift && ret)
			mapView.snapToGrid = !mapView.snapToGrid;
		if (snapping || mapView.snapToGrid)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(50);
			ImGui::DragInt("##gridSize", &mapView.gridSize, 1, 100);
			ImGui::SameLine();
			ImGui::Checkbox("##gridLocal", &mapView.gridLocal);
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();

		if (BrowEdit::editMode == EditMode::Object)
		{
			if (toolBarToggleButton("translate", 8, mapView.gadget.mode == Gadget::Mode::Translate))
				mapView.gadget.mode = Gadget::Mode::Translate;
			ImGui::SameLine();
			if (toolBarToggleButton("rotate", 9, mapView.gadget.mode == Gadget::Mode::Rotate))
				mapView.gadget.mode = Gadget::Mode::Rotate;
			ImGui::SameLine();
			if (toolBarToggleButton("scale", 10, mapView.gadget.mode == Gadget::Mode::Scale))
				mapView.gadget.mode = Gadget::Mode::Scale;
		}



		auto size = ImGui::GetContentRegionAvail();
		mapView.update(this, size);
		mapView.render(size.x / (float)size.y, config.fov);
		if(editMode == EditMode::Object)
			mapView.postRenderObjectMode(this);
		ImTextureID id = (ImTextureID)((long long)mapView.fbo->texid[0]); //TODO: remove cast for 32bit
		ImGui::Image(id, size, ImVec2(0,1), ImVec2(1,0));
		mapView.hovered = ImGui::IsItemHovered();
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
		ImGui::MenuItem("Save As", "Ctrl+s");
		if(ImGui::MenuItem("Quit"))
			glfwSetWindowShouldClose(window, 1);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+z"))
			undo();
		if (ImGui::MenuItem("Redo", "Ctrl+shift+z"))
			redo();
		if (ImGui::MenuItem("Undo Window", nullptr, windowData.undoVisible))
			windowData.undoVisible = !windowData.undoVisible;
		if (ImGui::MenuItem("Configure"))
			windowData.configVisible = true;
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Maps"))
	{
		for (auto map : maps)
		{
			if (ImGui::MenuItem(map->name.c_str()))
			{
				loadMap(map->name);
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

	if (toolBarToggleButton("texturemode", 4, editMode == EditMode::Texture))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("objectmode", 5, editMode == EditMode::Object))
		editMode = EditMode::Object;
	ImGui::SameLine();
	if (toolBarToggleButton("wallmode", 6, editMode == EditMode::Wall))
		editMode = EditMode::Wall;

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
	if (ImGui::Begin("Open Map", 0, ImGuiWindowFlags_NoDocking))
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

	}
	ImGui::End();
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
		if (mv.map == map && mv.viewName == file + "##" + std::to_string(viewCount))
			viewCount++;
	mapViews.push_back(MapView(map, file + "##" + std::to_string(viewCount)));
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
		int flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (std::find(selectedNodes.begin(), selectedNodes.end(), node) != selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
		if (ImGui::IsItemClicked())
		{
			selectedNodes.clear();
			selectedNodes.push_back(node);
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
		int flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
		if (std::find(selectedNodes.begin(), selectedNodes.end(), node) != selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
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
			selectedNodes.clear();
			selectedNodes.push_back(node);
		}
	}
}


void BrowEdit::showObjectProperties()
{
	ImGui::Begin("Properties");
	ImGui::PushFont(font);
	if (selectedNodes.size() == 1)
	{
		ImGui::InputText("Name", &selectedNodes[0]->name);
		for (auto c : selectedNodes[0]->components)
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
	if (ImGui::BeginListBox("##stack", ImGui::GetContentRegionAvail()))
	{
		for (auto action : undoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		for (auto action : redoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PopStyleColor();
		ImGui::EndListBox();
	}

	ImGui::End();
}