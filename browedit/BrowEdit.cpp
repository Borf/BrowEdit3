#include <Windows.h> //to remove ugly warning :(
#include "BrowEdit.h"
#include "Version.h"
#include <GLFW/glfw3.h>
#include <BugTrap.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <browedit/Map.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/NodeRenderer.h>
#include <browedit/Node.h>
#include <browedit/Gadget.h>
#include <browedit/Lightmapper.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>

#define Q(x) #x
#define QUOTE(x) Q(x)


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
	BT_SetAppVersion("V3." QUOTE(VERSION));
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
		try {
			tagList = util::FileIO::getJson("data\\tags.json").get<std::map<std::string, std::vector<std::string>>>();
			for (auto tag : tagList)
				for (const auto& t : tag.second)
					tagListReverse[util::utf8_to_iso_8859_1(t)].push_back(util::utf8_to_iso_8859_1(tag.first));
		}
		catch (...) {}
	}

	backgroundTexture = util::ResourceManager<gl::Texture>::load("data\\background.png");
	iconsTexture = util::ResourceManager<gl::Texture>::load("data\\icons.png");
	lightTexture = util::ResourceManager<gl::Texture>::load("data\\light.png");
	effectTexture = util::ResourceManager<gl::Texture>::load("data\\effect.png");
	soundTexture = util::ResourceManager<gl::Texture>::load("data\\sound.png");
	
	NodeRenderer::begin();
	Gadget::init();

#ifdef _DEBUG
	if(config.isValid() == "")
//		loadMap("data\\comodo.rsw");
//		loadMap("data\\guild_vs1.rsw");
//		loadMap("data\\effects_ro.rsw");
//		loadMap("data\\prt_in.rsw");
		loadMap("data\\wall_colour.rsw");
//		loadMap("data\\easter_la.rsw");
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
		if (windowData.objectWindowVisible && editMode == EditMode::Object)
			showObjectWindow();
		if (windowData.demoWindowVisible)
			ImGui::ShowDemoWindow(&windowData.demoWindowVisible);
		if (windowData.helpWindowVisible)
			showHelpWindow();

		showLightmapSettingsWindow();

		if (windowData.progressWindowVisible)
		{
			std::lock_guard<std::mutex> guard(windowData.progressMutex);
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
				if (ImGui::IsKeyPressed('Q') && activeMapView)
					activeMapView->map->recalculateQuadTree(this);
				if (ImGui::IsKeyPressed('L') && activeMapView)
				{
					lightmapper = new Lightmapper(activeMapView->map, this);
					windowData.openLightmapSettings = true;
				}
				if(ImGui::IsKeyPressed('R'))
					for (auto t : util::ResourceManager<gl::Texture>::getAll())
						t->reload();
			}
			if (ImGui::GetIO().KeyShift)
			{
				if (ImGui::IsKeyPressed('R'))
				{
					for (auto rsm : util::ResourceManager<Rsm>::getAll())
						rsm->reload();
					for (auto m : maps)
						m->rootNode->traverse([](Node* n) {
						auto rsmRenderer = n->getComponent<RsmRenderer>();
						if (rsmRenderer)
							rsmRenderer->begin();
							});
				}
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
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		float ratio = display_w / (float)display_h;
		if (display_h == 0)
			ratio = 1.0f;
		glViewport(0, 0, display_w, display_h);
		glClearColor(config.backgroundColor.r, config.backgroundColor.g, config.backgroundColor.b, 1);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glUseProgram(0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1*ratio, 1 * ratio, -1, 1, -100, 100);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3f(1, 1, 1);
		backgroundTexture->bind();
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);		glVertex3f(-1, -1, 0);
		glTexCoord2f(1, 1);		glVertex3f(1, -1, 0);
		glTexCoord2f(1, 0);		glVertex3f(1, 1, 0);
		glTexCoord2f(0, 0);		glVertex3f(-1, 1, 0);
		glEnd();
		imguiLoopEnd();
		glfwLoopEnd();
	}
	
	NodeRenderer::end();
	imguiEnd();
	glfwEnd();
}

void BrowEdit::configBegin()
{
	json configJson; //don't use util::FileIO::getJson for this, as the paths haven't been loaded in the FileIO yet
	std::ifstream configFile("config.json");
	if (configFile.is_open())
	{
		try {
			configFile >> configJson;
			configFile.close();
			config = configJson.get<Config>();
		}
		catch (...) {
			std::cerr << "Config file invalid, resetting config" << std::endl;
		}
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


ImVec4 enabledColor(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, 0.5f);
ImVec4 disabledColor(72 / 255.0f, 96 / 255.0f, 125 / 255.0f, 0.5f);

bool BrowEdit::toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip, ImVec4 tint)
{
	tint = ImVec4(config.toolbarButtonTint.x * tint.x, config.toolbarButtonTint.y * tint.y, config.toolbarButtonTint.z * tint.z, config.toolbarButtonTint.w * tint.w);
	ImVec2 v1((1.0f / iconsTexture->width) * (100 * (icon % 8)), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (100 * (icon / 8)));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 100, v1.y + (1.0f / iconsTexture->height) * 100);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, *status ? enabledColor : disabledColor, tint);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	if(clicked)
		*status = !*status;
	ImGui::PopID();
	return clicked;
}

bool BrowEdit::toolBarToggleButton(const std::string_view& name, int icon, bool status, const char* tooltip, ImVec4 tint)
{
	tint = ImVec4(config.toolbarButtonTint.x * tint.x, config.toolbarButtonTint.y * tint.y, config.toolbarButtonTint.z * tint.z, config.toolbarButtonTint.w * tint.w);
	ImVec2 v1((1.0f / iconsTexture->width) * (100 * (icon % 8)), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (100 * (icon / 8)));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 100, v1.y + (1.0f / iconsTexture->height) * 100);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, status ? enabledColor : disabledColor, tint);
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
			if (editMode == EditMode::Height)
				mapView.postRenderHeightMode(this);
			else if (editMode == EditMode::Object)
				mapView.postRenderObjectMode(this);
			mapView.prevMouseState = mapView.mouseState; //meh
		}

		ImTextureID id = (ImTextureID)((long long)mapView.fbo->texid[0]); //TODO: remove cast for 32bit
		ImGui::ImageButton(id, size, ImVec2(0,1), ImVec2(1,0), 0, ImVec4(0,0,0,1), ImVec4(1,1,1,1));
		//ImGui::Image(id, size, ImVec2(0, 1), ImVec2(1, 0));
		mapView.hovered = ImGui::IsItemHovered() || ImGui::IsItemClicked();
	}
	if (ImGui::IsWindowFocused())
	{
		if (activeMapView != &mapView)
			std::cout << "Switching map to " << mapView.viewName<<std::endl;
		activeMapView = &mapView;
	}
	ImGui::End();

}






void fixBackup(const std::string& fileName)
{
	std::ifstream is(fileName, std::ios_base::binary | std::ios_base::in);
	if (is.is_open())
	{
		int c = 0;
		for(int i = 0; i < 999; i++)
			if (!std::filesystem::exists(fileName + "." + std::to_string(i)))
			{
				c = i;
					break;
			}
		std::ofstream os((fileName + "." + std::to_string(c)), std::ios_base::binary | std::ios_base::out);
		char buf[1024];
		while (!is.eof())
		{
			is.read(buf, 1024);
			auto count = is.gcount();
			os.write(buf, count);
		}
	}
}


void BrowEdit::saveMap(Map* map)
{
	std::string rswName = config.ropath + map->name;
	std::string gndName = config.ropath + map->name.substr(0, map->name.size() - 4) + ".gnd";

	if (config.backup)
	{
		fixBackup(rswName);
		fixBackup(gndName);
	}

	map->rootNode->getComponent<Rsw>()->save(rswName);
	map->rootNode->getComponent<Gnd>()->save(gndName);
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