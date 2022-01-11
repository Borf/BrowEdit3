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


int main()
{
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
		windowData.configVisible = true;

	backgroundTexture = new gl::Texture("data\\background.png", false);
	iconsTexture = new gl::Texture("data\\icons.png", false);
	NodeRenderer::begin();

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

		imguiLoopEnd();
		glfwLoopEnd();
	}
	
	NodeRenderer::end();
	imguiEnd();
	glfwEnd();
}



bool BrowEdit::toolBarToggleButton(const char* name, int icon, bool &status)
{
	ImVec2 v1((1.0f / iconsTexture->width) * (36 * (icon%4) + 1.5f), //TODO: remove these hardcoded numbers
		(1.0f / iconsTexture->height) * (36 * (icon/4) + 1.5f));
	ImVec2 v2(v1.x + (1.0f / iconsTexture->width) * 34, v1.y + (1.0f / iconsTexture->height) * 34);
	ImGui::PushID(name);

	bool clicked = ImGui::ImageButton((ImTextureID)(long long)iconsTexture->id, ImVec2(32, 32), v1, v2, 0, ImVec4(144 / 255.0f, 193 / 255.0f, 249 / 255.0f, status ? 1.0f : 0.0f));
	if(clicked)
		status = !status;
	ImGui::PopID();
	return clicked;
}

void BrowEdit::showMapWindow(MapView& mapView)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(2048, 2048));
	if (ImGui::Begin(mapView.viewName.c_str(), &mapView.opened))
	{
		toolBarToggleButton("viewLightMapShadow", 0, mapView.viewLightmapShadow);
		ImGui::SameLine();
		toolBarToggleButton("viewLightmapColor", 1, mapView.viewLightmapColor);
		ImGui::SameLine();
		toolBarToggleButton("viewColors", 2, mapView.viewColors);
		ImGui::SameLine();
		toolBarToggleButton("viewLighting", 3, mapView.viewLighting);


		auto size = ImGui::GetContentRegionAvail();
		mapView.update(this);
		mapView.render(size.x / (float)size.y, config.fov);
		ImTextureID id = (ImTextureID)((long long)mapView.fbo->texid[0]); //TODO: remove cast for 32bit
		ImGui::Image(id, size, ImVec2(0,1), ImVec2(1,0));
	}
	ImGui::End();
}



void BrowEdit::menuBar()
{
	ImGui::BeginMainMenuBar();
	menubarHeight = ImGui::GetWindowSize().y;
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New");
		if (ImGui::MenuItem("Open"))
		{
			windowData.openFiles = util::FileIO::listFiles("data");
			windowData.openFiles.erase(std::remove_if(windowData.openFiles.begin(), windowData.openFiles.end(), [](const std::string& map) { return map.substr(map.size()-4, 4) != ".rsw"; }), windowData.openFiles.end());
			windowData.openVisible = true;
			
		}
		ImGui::MenuItem("Save As");
		if(ImGui::MenuItem("Quit"))
			glfwSetWindowShouldClose(window, 1);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
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

	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menubarHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));
	ImGuiWindowFlags toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Toolbar", 0, toolbarFlags);

	ImGui::Text("Texture");
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));
	if (ImGui::Button("Texture"))
	{

	}
	ImGui::PopStyleColor(3);
	ImGui::SameLine();


	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.5f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.5f, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.5f, 0.8f, 0.8f));
	if (ImGui::Button("Model"))
	{

	}
	ImGui::PopStyleColor(3);
	ImGui::SameLine();

	ImGui::Separator();

	float f = 0;
	char s[1024] = "test";
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

