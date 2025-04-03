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
#include <browedit/HotkeyRegistry.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/NodeRenderer.h>
#include <browedit/Node.h>
#include <browedit/Gadget.h>
#include <browedit/Lightmapper.h>
#include <browedit/ModelEditor.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gat.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/util/Console.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/util/glfw_keycodes_to_string.h>

#define Q(x) #x
#define QUOTE(x) Q(x)


#ifdef _WIN32
	#include <Windows.h> //to remove ugly warning :(

	extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

#ifndef _DEBUG
#pragma comment(lib, "BugTrap-x64.lib") // Link to ANSI DLL
#endif

void fixEffectPreviews();

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

	ConsoleInject consoleInjector;

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
  :xlcOk:,'                        Borf's Ragnarok Online World Editor, v3
   ,clo:                                                                        


)V0G0N" << std::endl;

	BrowEdit().run();
}

std::map<Map*, MapView*> firstRender; //DIRTY HACK
BrowEdit* browEdit; //EVEN DIRTIER HACK

void BrowEdit::run()
{
	browEdit = this;
	configBegin();
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
	config.setStyle(config.style);

	editMode = (EditMode)config.defaultEditMode;
	if (config.closeObjectWindowOnAdd)
		windowData.objectWindowVisible = false;
	registerActions();
	HotkeyRegistry::init(config.hotkeys);
	{
		try {
			tagList = util::FileIO::getJson("data\\tags.json").get<std::map<std::string, std::vector<std::string>>>();
			for (auto tag : tagList)
				for (const auto& t : tag.second)
					tagListReverse[util::tolower(util::utf8_to_iso_8859_1(t))].push_back(util::utf8_to_iso_8859_1(tag.first));
		}
		catch (...) {}
	}

	backgroundTexture = util::ResourceManager<gl::Texture>::load("data\\background.png");
	iconsTexture = util::ResourceManager<gl::Texture>::load("data\\icons.png");
	lightTexture = util::ResourceManager<gl::Texture>::load("data\\light.png");
	effectTexture = util::ResourceManager<gl::Texture>::load("data\\effect.png");
	soundTexture = util::ResourceManager<gl::Texture>::load("data\\sound.png");
	gatTexture = util::ResourceManager<gl::Texture>::load("data\\gat.png");
	prefabTexture = util::ResourceManager<gl::Texture>::load("data\\prefab.png");


	//fixEffectPreviews();


	NodeRenderer::begin();
	Gadget::init();

#ifdef _DEBUG
	if(config.isValid() == "")
//		loadMap("data\\aldebaran.rsw");
//		loadMap("data\\prontera.rsw");
//		loadMap("data\\amicit01.rsw"); //RSM2
//		loadMap("data\\grademk.rsw"); //special effects
//		loadMap("data\\noel02.rsw");
//		loadMap("data\\icecastle.rsw");
//		loadMap("data\\bl_ice.rsw");
//		loadMap("data\\comodo.rsw");
//		loadMap("data\\guild_vs1.rsw");
//		loadMap("data\\effects_ro.rsw");
//		loadMap("data\\prt_in.rsw");
		loadMap("data\\wall_colour.rsw");
//		loadMap("data\\untomb_05s.rsw");
//		loadMap("data\\easter_la.rsw");
//		loadMap("data\\2@alice_mad.rsw");
//		loadMap("data\\prt_vilg01.rsw");
//		loadMap("data\\dae_paysq.rsw");
//		loadMap("data\\veins.rsw");
//		loadMap("data\\rag_fes.rsw");
//		loadMap("data\\justincase.rsw");
//		loadMap("data\\market_xmas.rsw");
//		loadMap("data\\arena2.rsw");
//		loadMap("data\\maze_new.rsw");

		//loadModel("data\\model\\prontera_re\\streetlamp_01.rsm");
	//	loadModel("data\\model\\크리스마스마을\\xmas_내부트리.rsm");
	//	loadModel("data\\model\\인던02\\인던02b중앙장식01.rsm");
		//loadModel("data\\model\\event\\3차전직_석상02.rsm"); //bigass statue
		//loadModel("data\\model\\para\\alchemy_01.rsm");
		//loadModel("data\\model\\para\\mora_01.rsm");
		//loadModel("data\\model\\para\\mora_02.rsm");
		//loadModel("data\\model\\masin\\fire_land.rsm");
		//loadModel("data\\model인던02인던02미이라.rsm");
		//loadModel("data\\model\\pud\\stall_01.rsm");
		//loadModel("data\\model\\pud\\stall_02.rsm");
		//loadModel("data\\model\\pud\\stall_03.rsm");
		//loadModel("data\\model\\pud\\swing_01.rsm");
		//loadModel("data\\model\\pud\\balloon_01.rsm");
		//loadModel("data\\model\\plants_e_01.rsm2");
	//	modelEditor.load("data\\model\\para\\mora_01.rsm");
		//modelEditor.load("data\\model\\프론테라\\교역소.rsm");
		//modelEditor.load("data\\model\\ilusion\\goldberg_s_01.rsm2");
		//modelEditor.load("data\\model\\job4for\\purifier_s_01.rsm2");
#endif


	double time = ImGui::GetTime();
	while (true)
	{
		if (!glfwLoopBegin())
			break;
		imguiLoopBegin();

		if (cursor != nullptr)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursor(window, cursor);
			cursor = nullptr;
		}

		double newTime = ImGui::GetTime();
		double deltaTime = newTime - time;
		time = newTime;

		menuBar();
		toolbar();

		if (windowData.configVisible)
			windowData.configVisible = !config.showWindow(this);

		if (windowData.showNewMapPopup)
		{
			windowData.showNewMapPopup = false;
			ImGui::OpenPopup("NewMapPopup");
		}
		ShowNewMapPopup();

		if (windowData.showHotkeyPopup)
		{
			windowData.showHotkeyPopup = false;
			windowData.hotkeyPopupFilter = "";
			ImGui::OpenPopup("HotkeyPopup");
		}
		HotkeyRegistry::showHotkeyPopup(this);

		openWindow();
		showExportWindow();
		if (windowData.undoVisible)
			showUndoWindow();
		if (windowData.objectWindowVisible && editMode == EditMode::Object)
			showObjectWindow();
		if (editMode == EditMode::Object)
			showObjectEditToolsWindow();
		if (windowData.demoWindowVisible)
			ImGui::ShowDemoWindow(&windowData.demoWindowVisible);
		if (windowData.helpWindowVisible)
			showHelpWindow();
		if (editMode == EditMode::Texture)
		{
			showTextureBrushWindow();
			showTextureManageWindow();
		}
		if (editMode == EditMode::Height)
			showHeightWindow();
		if (editMode == EditMode::Gat)
			showGatWindow();
		if (editMode == EditMode::Wall)
		{
			showWallWindow();
			showTextureManageWindow();
		}
		if (editMode == EditMode::Color)
			showColorEditWindow();
		if (editMode == EditMode::Shadow)
			showShadowEditWindow();
		if (editMode == EditMode::Cinematic)
			showCinematicModeWindow();

		if (windowData.hotkeyEditWindowVisible)
			showHotkeyEditorWindow();

		showLightmapSettingsWindow();

		if (windowData.cropWindowVisible)
			showCropSettingsWindow();

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
				if (windowData.progressCancel != nullptr)
				{
					if (ImGui::Button("Cancel"))
					{
						windowData.progressCancel();
					}
				}
				ImGui::EndPopup();
			}
		}
		else if (windowData.progressWindowProgres >= 1.0f)
		{
			windowData.progressWindowProgres = 0;
			if (windowData.progressWindowOnDone)
				windowData.progressWindowOnDone();
			windowData.progressWindowOnDone = nullptr;
			windowData.progressCancel = nullptr;
		}


		if (editMode == EditMode::Object)
		{
			showObjectTree();
			showObjectProperties();
		}

		firstRender.clear(); //DIRTY HACK
		for (auto it = mapViews.begin(); it != mapViews.end(); )
		{
			showMapWindow(*it, (float)deltaTime);
			if (!it->opened)
			{
				if (activeMapView == &(*it))
					activeMapView = nullptr;
				if (textureStampMap == it->map)
					textureStampMap = nullptr;
				Map* map = it->map;
				it = mapViews.erase(it);

				auto count = std::count_if(mapViews.begin(), mapViews.end(), [map](const MapView& mv) { return mv.map == map; });
				if (count == 0)
				{
					std::erase_if(maps, [map](Map* m) { return m == map;  });
					delete map;
				}
			}
			else
				it++;
		}

		modelEditor.run(this);

		if (windowData.showQuitConfirm)
		{
			windowData.showQuitConfirm = false;
			ImGui::OpenPopup("QuitConfirm");
		}
		if (ImGui::BeginPopupModal("QuitConfirm"))
		{
			ImGui::Text("There are unsaved changes in one of your maps. Are you sure you want to quit?");
			if (ImGui::Button("Save & Quit"))
			{
				for (auto m : maps)
					if (m->changed)
						saveMap(m);
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::SameLine();
			if (ImGui::Button("Quit"))
				glfwSetWindowShouldClose(window, GLFW_TRUE);

			ImGui::EndPopup();
		}




		if (!ImGui::GetIO().WantTextInput)
		{
			HotkeyRegistry::checkHotkeys();
			if (editMode == EditMode::Height && activeMapView)
			{
				if (!heightDoodle)
				{
					if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
						activeMapView->map->growTileSelection(this);
					if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
						activeMapView->map->shrinkTileSelection(this);
				}

			}
			//TODO: move to method
			if (ImGui::BeginPopupModal("PasteSpecial"))
			{
				static int pasteOptions = -1;
				ImGui::CheckboxFlags("Paste Heights", &pasteOptions, PasteOptions::Height);
				ImGui::CheckboxFlags("Paste Walls", &pasteOptions, PasteOptions::Walls);
				ImGui::CheckboxFlags("Paste Textures", &pasteOptions, PasteOptions::Textures);
				ImGui::CheckboxFlags("Paste Colors", &pasteOptions, PasteOptions::Colors);
				ImGui::CheckboxFlags("Paste Lightmaps", &pasteOptions, PasteOptions::Lightmaps);
				ImGui::CheckboxFlags("Paste Objects", &pasteOptions, PasteOptions::Objects);
				ImGui::CheckboxFlags("Paste GAT", &pasteOptions, PasteOptions::GAT);
				if (ImGui::Button("Paste"))
				{
					this->pasteOptions = pasteOptions;
					pasteTiles();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
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
		glDepthFunc(GL_LESS);
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
			config.defaultHotkeys();
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
		config.defaultHotkeys();
		windowData.configVisible = true;
		util::FileIO::begin();
		util::FileIO::addDirectory(".\\");
		util::FileIO::end();
	}
}


ImVec4 enabledColor(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, 0.5f);
ImVec4 disabledColor(72 / 255.0f, 96 / 255.0f, 125 / 255.0f, 0.5f);

bool BrowEdit::toolBarToggleButton(const std::string_view& name, int icon, bool status, const char* tooltip, HotkeyAction action, ImVec4 tint)
{
	std::string strTooltip = tooltip;
	auto hk = HotkeyRegistry::getHotkey(action);
	if (hk.hotkey.keyCode != 0)
		strTooltip += "(" + hk.hotkey.toString() + ")";
	bool clicked = toolBarToggleButton(name, icon, status, strTooltip.c_str(), tint);
	if(clicked)
		HotkeyRegistry::runAction(action);
	return clicked;
}


bool BrowEdit::toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip, ImVec4 tint)
{
	tint = ImVec4(config.toolbarButtonTint.x * tint.x, config.toolbarButtonTint.y * tint.y, config.toolbarButtonTint.z * tint.z, config.toolbarButtonTint.w * tint.w);
	ImVec2 v1((1.0f / iconsTexture->width) * (100 * (icon % 10)), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (100 * (icon / 10)));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 100, v1.y + (1.0f / iconsTexture->height) * 100);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id(), ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, *status ? enabledColor : disabledColor, tint);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	if(clicked)
		*status = !*status;
	ImGui::PopID();
	return clicked;
}

bool BrowEdit::toolBarButton(const std::string_view& name, int icon, const char* tooltip, ImVec4 tint)
{
	tint = ImVec4(config.toolbarButtonTint.x * tint.x, config.toolbarButtonTint.y * tint.y, config.toolbarButtonTint.z * tint.z, config.toolbarButtonTint.w * tint.w);
	ImVec2 v1((1.0f / iconsTexture->width) * (100 * (icon % 10)), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (100 * (icon / 10)));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 100, v1.y + (1.0f / iconsTexture->height) * 100);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id(), ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, enabledColor, tint);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	ImGui::PopID();
	return clicked;
}


bool BrowEdit::toolBarToggleButton(const std::string_view& name, int icon, bool status, const char* tooltip, ImVec4 tint)
{
	tint = ImVec4(config.toolbarButtonTint.x * tint.x, config.toolbarButtonTint.y * tint.y, config.toolbarButtonTint.z * tint.z, config.toolbarButtonTint.w * tint.w);
	ImVec2 v1((1.0f / iconsTexture->width) * (100 * (icon % 10)), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (100 * (icon / 10)));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 100, v1.y + (1.0f / iconsTexture->height) * 100);
	ImGui::PushID(name.data());

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id(), ImVec2(config.toolbarButtonSize, config.toolbarButtonSize), v1, v2, 0, status ? enabledColor : disabledColor, tint);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(tooltip);
	ImGui::PopID();
	return clicked;
}
void BrowEdit::showMapWindow(MapView& mapView, float deltaTime)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(2048, 2048));
	int flags = 0;
	if (mapView.map->changed)
		flags |= ImGuiWindowFlags_UnsavedDocument;
	if (ImGui::Begin(mapView.viewName.c_str(), &mapView.opened, flags))
	{
		mapView.toolbar(this);
		auto size = ImGui::GetContentRegionAvail();
		if (mapView.map->rootNode->getComponent<Gnd>())
		{
			mapView.update(this, size, deltaTime);
			if (firstRender.find(mapView.map) != firstRender.end()) //TODO: THIS IS A DIRTY HACK
			{
				mapView.nodeRenderContext.renderers = firstRender[mapView.map]->nodeRenderContext.renderers;
				mapView.nodeRenderContext.ordered = firstRender[mapView.map]->nodeRenderContext.ordered;
			}
			mapView.render(this);
			bool cameraWidgetClicked = mapView.drawCameraWidget();
			bool clicked = ImGui::IsMouseClicked(0);
			firstRender[mapView.map] = &mapView;
			if (!cameraWidgetClicked)
			{
				if (editMode == EditMode::Height)
					mapView.postRenderHeightMode(this);
				else if (editMode == EditMode::Object)
					mapView.postRenderObjectMode(this);
				else if (editMode == EditMode::Texture)
					mapView.postRenderTextureMode(this);
				else if (editMode == EditMode::Gat)
					mapView.postRenderGatMode(this);
				else if (editMode == EditMode::Wall)
					mapView.postRenderWallMode(this);
				else if (editMode == EditMode::Color)
					mapView.postRenderColorMode(this);
				else if (editMode == EditMode::Shadow)
					mapView.postRenderShadowMode(this);
				else if (editMode == EditMode::Cinematic)
					mapView.postRenderCinematicMode(this);
			}


			mapView.prevMouseState = mapView.mouseState; //meh
		}

		ImTextureID id = (ImTextureID)((long long)mapView.fbo->texid[0]); //TODO: remove cast for 32bit
		ImGui::ImageButton(id, size, ImVec2(0,1), ImVec2(1,0), 0, ImVec4(0,0,0,1), ImVec4(1,1,1,1));
		//ImGui::Image(id, size, ImVec2(0, 1), ImVec2(1, 0));
		mapView.hovered = ImGui::IsItemHovered() || ImGui::IsItemClicked();

		if (&mapView == activeMapView)
		{

		}

	}
	if (ImGui::IsWindowFocused() || (ImGui::IsWindowHovered() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2))))
	{
		if (activeMapView != &mapView)
			std::cout << "Switching map to " << mapView.viewName<<std::endl;
		activeMapView = &mapView;
	}
	ImGui::End();

}






void fixBackup(const std::string& fileName, const std::string& backupfileName)
{
	std::ifstream is(fileName, std::ios_base::binary | std::ios_base::in);
	if (is.is_open())
	{
		std::filesystem::path path(backupfileName);
		auto p = path.parent_path().string();
		try {
			if (!std::filesystem::exists(p))
				std::filesystem::create_directories(p);
		}
		catch (...)
		{
			std::cerr << "Trying to create path " << p << ", but this didn't work for some reason" << std::endl;
		}

		int c = 0;
		for(int i = 0; i < 99999; i++)
			if (!std::filesystem::exists(backupfileName + "." + std::to_string(i)))
			{
				c = i;
					break;
			}
		std::ofstream os((backupfileName + "." + std::to_string(c)), std::ios_base::binary | std::ios_base::out);
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
	if(config.recalculateQuadtreeOnSave)
		map->recalculateQuadTree(this);

	std::string mapName = map->name;
	bool fullPath = false;
	if (mapName.find(":") != std::string::npos)
		fullPath = true;

	if (mapName.find(".rsw") != std::string::npos)
		mapName = mapName.substr(0, mapName.find(".rsw"));
	if (mapName.find("\\") != std::string::npos)
		mapName = mapName.substr(mapName.rfind("\\") + 1);

	std::string rswName = config.ropath + map->name;
	std::string gndName = config.ropath + map->name.substr(0, map->name.size() - 4) + ".gnd";
	std::string gatName = config.ropath + map->name.substr(0, map->name.size() - 4) + ".gat";
	std::string lubName = config.ropath + "data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub";
	if (fullPath)
	{
		rswName = map->name;
		gndName = map->name.substr(0, map->name.size() - 4) + ".gnd";
		//dunno what to do with lub
	}

	std::string backupRswName = "backups\\" + map->name;
	std::string backupGndName = "backups\\" + map->name.substr(0, map->name.size() - 4) + ".gnd";
	std::string backupGatName = "backups\\" + map->name.substr(0, map->name.size() - 4) + ".gat";
	std::string backupLubName = "backups\\data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub";
	if (fullPath)
	{
		rswName = map->name;
		gndName = map->name.substr(0, map->name.size() - 4) + ".gnd";
		gatName = map->name.substr(0, map->name.size() - 4) + ".gat";
		//dunno what to do with lub
	}


	if (config.backup)
	{
		fixBackup(rswName, backupRswName);
		fixBackup(gndName, backupGndName);
		fixBackup(gatName, backupGatName);
		fixBackup(lubName, backupLubName);
	}

	map->rootNode->getComponent<Rsw>()->save(rswName, this);
	map->rootNode->getComponent<Gnd>()->save(gndName, map->rootNode->getComponent<Rsw>());
	map->rootNode->getComponent<Gat>()->save(gatName);

	map->changed = false;
}

void BrowEdit::saveAsMap(Map* map)
{
	std::string path = util::SaveAsDialog(config.ropath + map->name, "Rsw\0*.rsw\0");
	if (path == "")
		return;

	if (path.size() < 4 || path.substr(path.size() - 4) != ".rsw")
		path += ".rsw";

	if (path.find(config.ropath) == std::string::npos)
	{
		std::cout << "WARNING: YOU ARE SAVING OUTSIDE OF THE RO DATA PATH. THIS WILL GIVE BUGS" << std::endl;
		map->name = path;
	}
	else
	{
		map->name = path.substr(path.find(config.ropath) + config.ropath.size());
		for (auto& mv : mapViews)
		{
			if (mv.map == map)
			{
				mv.viewName = mv.map->name + mv.viewName.substr(mv.viewName.find("#"));
			}
		}
	}
	if (config.recalculateQuadtreeOnSave)
		map->recalculateQuadTree(this);

	std::string fileName = path.substr(path.rfind("\\") + 1);
	std::string directory = path.substr(0, path.rfind("\\")+1);

	std::string mapName = fileName;
	if (mapName.find(".rsw") != std::string::npos)
		mapName = mapName.substr(0, mapName.find(".rsw"));

	std::string rswName = directory + mapName + ".rsw";
	std::string gndName = directory + mapName + ".gnd";
	std::string gatName = directory + mapName + ".gat";
	//	std::string lubName = config.ropath + "data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub"; //not sure where to store this

	std::string backupRswName = "backups\\" + map->name;
	std::string backupGndName = "backups\\" + map->name.substr(0, map->name.size() - 4) + ".gnd";
	std::string backupGatName = "backups\\" + map->name.substr(0, map->name.size() - 4) + ".gat";
	std::string backupLubName = "backups\\data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub";

	if (config.backup)
	{
		fixBackup(rswName, backupRswName);
		fixBackup(gndName, backupGndName);
		fixBackup(gatName, backupGatName);
//		fixBackup(lubName, backupGndName);
	}

	map->rootNode->getComponent<Rsw>()->gatFile = mapName + ".gat";
	map->rootNode->getComponent<Rsw>()->gndFile = mapName + ".gnd";
	map->rootNode->getComponent<Rsw>()->save(rswName, this);
	map->rootNode->getComponent<Gnd>()->save(gndName, map->rootNode->getComponent<Rsw>());
	map->rootNode->getComponent<Gat>()->save(gatName);
}

void BrowEdit::loadMap(const std::string file)
{
	if (!util::FileIO::exists(file))
		return;

	if (std::find(config.recentFiles.begin(), config.recentFiles.end(), file) != config.recentFiles.end())
		std::erase_if(config.recentFiles, [file](const std::string s) { return s == file; });
	config.recentFiles.insert(config.recentFiles.begin(), file);
	if (config.recentFiles.size() > 10)
		config.recentFiles.resize(10);
	config.save();

	Map* map = nullptr;
	for (auto m : maps)
		if (m->name == file)
			map = m;
	if (!map)
	{
		map = new Map(file, this);
		maps.push_back(map);
	}
	int viewCount = 0;
	for (const auto& mv : mapViews)
		if (mv.map == map && mv.viewName == file + "#" + std::to_string(viewCount))
			viewCount++;
	mapViews.push_back(MapView(map, file + "#" + std::to_string(viewCount)));
}


void BrowEdit::copyTiles()
{
	auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
	auto gat = activeMapView->map->rootNode->getComponent<Gat>();
	json clipboard;
	glm::ivec2 center(0);
	for (auto n : activeMapView->map->tileSelection)
		center += n;
	center /= activeMapView->map->tileSelection.size();
	clipboard["center"] = center;
	for (auto n : activeMapView->map->tileSelection)
	{
		auto c = gnd->cubes[n.x][n.y];
		json cube = *c;
		cube["pos"] = n - center;
		clipboard["cubes"].push_back(cube);
		for (int i = 0; i < 4; i++)
		{
			if (gat->inMap(n * 2 + glm::ivec2(i%2, i/2)))
			{
				json cube = *gat->cubes[n.x * 2 + (i % 2)][n.y * 2 + (i / 2)];
				cube["pos"] = (n * 2 + glm::ivec2(i % 2, i / 2)) - 2*center;
				clipboard["gats"].push_back(cube);
			}
		}		

		for(int i = 0; i < 3; i++)
			if (c->tileIds[i] != -1)
			{
				auto tile = gnd->tiles[c->tileIds[i]];
				for (int ii = 0; ii < 4; ii++)
					for (int iii = 0; iii < 2; iii++)
						if (std::isnan(tile->texCoords[ii][iii]))
							tile->texCoords[ii][iii] = 0;
				clipboard["tiles"][std::to_string(c->tileIds[i])] = json(*tile);
				clipboard["textures"][std::to_string(tile->textureIndex)] = json(*gnd->textures[tile->textureIndex]);
				if (tile->lightmapIndex > -1)
				{
					clipboard["lightmaps"][std::to_string(tile->lightmapIndex)] = json(*gnd->lightmaps[tile->lightmapIndex]);
					clipboard["lightmaps"][std::to_string(tile->lightmapIndex)]["width"] = gnd->lightmapWidth;
					clipboard["lightmaps"][std::to_string(tile->lightmapIndex)]["height"] = gnd->lightmapHeight;
				}
			}
	}

	glm::vec3 centerObjects(center.x * 10 - 5 * gnd->width, 0, center.y * 10 - 5 * gnd->height);
	clipboard["objects"] = json::array();
	activeMapView->map->rootNode->traverse([&](Node* n) {
		auto rswObject = n->getComponent<RswObject>();
		if (rswObject)
		{
			glm::vec3 pos(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -(-10 - 5 * gnd->height + rswObject->position.z));
			for (auto tile : activeMapView->map->tileSelection)
			{
				if (pos.x >= tile.x * 10 && pos.x <= (tile.x + 1) * 10 &&
					pos.z >= 10 * gnd->height - (tile.y + 1) * 10 + 10 && pos.z <= 10 * gnd->height - (tile.y + 0) * 10 + 10)
				{
					clipboard["objects"].push_back(*n);
					clipboard["objects"][clipboard["objects"].size() - 1]["relative_position"] = rswObject->position - centerObjects;
					break;
				}
			}
		}
	});


	ImGui::SetClipboardText(clipboard.dump(1).c_str());
}
void BrowEdit::copyGat()
{
	auto gat = activeMapView->map->rootNode->getComponent<Gat>();
	json clipboard;
	glm::ivec2 center(0);
	for (auto n : activeMapView->map->gatSelection)
		center += n;
	center /= activeMapView->map->gatSelection.size();
	for (auto n : activeMapView->map->gatSelection)
	{
		auto c = gat->cubes[n.x][n.y];
		json cube = *c;
		cube["pos"] = n - center;
		clipboard["gats"].push_back(cube);
	}
	ImGui::SetClipboardText(clipboard.dump(1).c_str());
}
void BrowEdit::pasteTiles()
{
	try
	{
		auto c = ImGui::GetClipboardText();
		if (c == nullptr)
			return;
		std::string cb = c;
		if (cb == "")
			return;
		json clipboard = json::parse(cb);
		pasteData = clipboard; //TODO: don't make a copy
		if (clipboard.size() > 0)
		{
			for (auto jsonCube : clipboard["cubes"])
			{
				auto cube = new CopyCube();
				from_json(jsonCube, *cube);
				for (int i = 0; i < 3; i++)
				{
					if (cube->tileIds[i] > -1)
					{
						try {
							from_json(clipboard["tiles"][std::to_string(cube->tileIds[i])], cube->tile[i]);
							if (cube->tile[i].lightmapIndex > -1)
							{
								cube->lightmap[i].gnd = activeMapView->map->rootNode->getComponent<Gnd>();
								from_json(clipboard["lightmaps"][std::to_string(cube->tile[i].lightmapIndex)], cube->lightmap[i]);
							}
							if (cube->tile[i].textureIndex > -1)
								from_json(clipboard["textures"][std::to_string(cube->tile[i].textureIndex)], cube->texture[i]);
						}
						catch (const std::exception &ex)
						{
							std::cout << ex.what() << std::endl;
							cube->tile[i].textureIndex = 0;
							cube->tile[i].lightmapIndex = 0; //wtf
						}
					}
				}
				newCubes.push_back(cube); //only used here, so when pasting it's safe to assume tile[i] has been filled
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void BrowEdit::pasteGat()
{
	try
	{
		auto c = ImGui::GetClipboardText();
		if (c == nullptr)
			return;
		std::string cb = c;
		if (cb == "")
			return;
		json clipboard = json::parse(cb);
		if (clipboard.size() > 0)
		{
			for (auto jsonCube : clipboard["gats"])
			{
				auto cube = new CopyCubeGat();
				cube->calcNormal();
				from_json(jsonCube, *cube);
				newGatCubes.push_back(cube);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}



void BrowEdit::saveTagList()
{
	json tagListJson = tagList;
	std::ofstream tagListFile("data\\tags.json");
	tagListFile << std::setw(2) << tagListJson;
}


bool BrowEdit::hotkeyInputBox(const char* title, Hotkey& hotkey)
{
	std::string text = hotkey.toString();

	ImGui::InputText(title, &text, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::IsItemFocused())
	{
		for (int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().KeysDown); i++)
		{
			if (ImGui::IsKeyPressed(i) && i != 340 && i != 341 && i != 342)
			{
				hotkey.keyCode = i;
				hotkey.modifiers = ImGui::GetIO().KeyMods;
				return true;
			}
		}
		
	}
	return false;
}

void BrowEdit::ShowNewMapPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 0));
	if (ImGui::BeginPopupModal("NewMapPopup"))
	{
		ImGui::Text("This is GND size, not GAT size");
		ImGui::Text("Your map will be twice the size in RO ingame coordinats");
		ImGui::InputInt("Width", &windowData.newMapWidth);
		ImGui::InputInt("Height", &windowData.newMapHeight);
		if (windowData.newMapWidth % 2 == 1 || windowData.newMapHeight % 2 == 1)
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Warning, maps with odd dimentions do not work well ingame");
		ImGui::Text("Your map will be twice the size in RO ingame coordinats");
		ImGui::InputText("Name", &windowData.newMapName);
		if (ImGui::Button("Create"))
		{
			Map* map = new Map("data\\" + windowData.newMapName + ".rsw", windowData.newMapWidth, windowData.newMapHeight, this);
			maps.push_back(map);
			mapViews.push_back(MapView(map, "data\\" + windowData.newMapName + ".rsw#0"));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}


#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <filesystem>

void fixEffectPreviews()
{
	try
	{
		for (auto entry : std::filesystem::directory_iterator("data\\texture\\effect", std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied))
		{
			if (entry.path().string().find(".png") != std::string::npos)
				continue;
			if (entry.path().string().find(".gif") == std::string::npos)
				continue;
			std::istream* is = util::FileIO::open(entry.path().string());
			std::cout << entry.path().string() << std::endl;
			if (!is)
			{
				std::cerr << "Texture: Could not open " <<entry.path().string() << std::endl;
				return;
			}
			is->seekg(0, std::ios_base::end);
			std::size_t len = is->tellg();
			if (len <= 0 || len > 100 * 1024 * 1024)
			{
				std::cerr << "Texture: Error opening texture " << entry.path().string() << ", file is either empty or too large" << std::endl;
				delete is;
				return;
			}
			char* buffer = new char[len];
			is->seekg(0, std::ios_base::beg);
			is->read(buffer, len);
			delete is;

			int width, height, frameCount, comp;
			int* delays;
			auto data = stbi_load_gif_from_memory((stbi_uc*)buffer, (int)len, &delays, &width, &height, &frameCount, &comp, 4);
			if (!data)
				continue;
			int bestFrame = 0;
			int bestFrameCount = 0;
			for (int f = 0; f < frameCount; f++)
			{
				auto d = data + (width * height * 4) * f;
				int frameIntensity = 0;
				for (int x = 0; x < width; x++)
				{
					for (int y = 0; y < height; y++)
					{
						frameIntensity += d[(x + width * y) * 4 + 0];
						frameIntensity += d[(x + width * y) * 4 + 1];
						frameIntensity += d[(x + width * y) * 4 + 2];
					}
				}
				if (frameIntensity > bestFrameCount)
				{
					bestFrameCount = frameIntensity;
					bestFrame = f;
				}
			}
			stbi_write_png((entry.path().string() + ".png").c_str(), width, height, 4, data + bestFrame * width * height*4, 0);

			std::cout << "Best frame is " << bestFrame << std::endl;


			stbi_image_free(data);
		}
		std::cout << "Done" << std::endl;
	}
	catch (const std::system_error& exception)
	{
		std::cerr << "data\\texture\\effect: " << exception.what() << " (" << exception.code() << ")" << std::endl;
	}
}
