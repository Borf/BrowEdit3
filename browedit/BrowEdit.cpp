#include <Windows.h> //to remove ugly warning :(
#include "BrowEdit.h"
#include <GLFW/glfw3.h>
#include <BugTrap.h>

#include <iostream>
#include <fstream>
#include <thread>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <browedit/Map.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/NodeRenderer.h>
#include <browedit/Node.h>
#include <browedit/Gadget.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>


#ifdef _WIN32
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

#ifndef _DEBUG
#pragma comment(lib, "BugTrap-x64.lib") // Link to ANSI DLL
#endif

static void SetupExceptionHandler()
{
	BT_InstallSehFilter();
	BT_SetAppName("Browedit");
	BT_SetFlags(BTF_DETAILEDMODE | BTF_SCREENCAPTURE);
	BT_SetSupportURL("https://discord.gg/bQaj5dKtbV");
}


int main()
{
#ifndef _DEBUG
	SetupExceptionHandler();
#endif

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

	backgroundTexture = util::ResourceManager<gl::Texture>::load("data\\background.png");
	iconsTexture = util::ResourceManager<gl::Texture>::load("data\\icons.png");
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

		openWindow();
		if (windowData.undoVisible)
			showUndoWindow();
		if (windowData.objectWindowVisible)
			showObjectWindow();
		if (windowData.demoWindowVisible)
			ImGui::ShowDemoWindow(&windowData.demoWindowVisible);

		if (windowData.progressWindowVisible)
		{
			ImGui::OpenPopup("Progress");
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(300, 0));
			if (ImGui::BeginPopupModal("Progress", 0, ImGuiWindowFlags_NoDocking))
			{
				ImGui::Text(windowData.progressWindowText.c_str());
				ImGui::ProgressBar(windowData.progressWindowProgres);
				ImGui::EndPopup();
			}
		}
		else if (windowData.progressWindowProgres >= 1.0f)
		{
			windowData.progressWindowProgres = 0;
			windowData.progressWindowOnDone();
			windowData.progressWindowOnDone = nullptr;
		}


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
				if (ImGui::IsKeyPressed('O'))
					showOpenWindow();
			}
			if (editMode == EditMode::Object && activeMapView)
			{
				if (ImGui::IsKeyPressed(GLFW_KEY_DELETE) && activeMapView)
					activeMapView->map->deleteSelection(this);
				if (ImGui::IsKeyPressed('F'))
					activeMapView->focusSelection();

				if (ImGui::GetIO().KeyCtrl)
				{
					if (ImGui::IsKeyPressed('C'))
						activeMapView->map->copySelection();
					if (ImGui::IsKeyPressed('V'))
						activeMapView->map->pasteSelection(this);
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
		mapView.toolbar(this);


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






void BrowEdit::saveTagList()
{
	json tagListJson = tagList;
	std::ofstream tagListFile("data\\tags.json");
	tagListFile << std::setw(2) << tagListJson;
}