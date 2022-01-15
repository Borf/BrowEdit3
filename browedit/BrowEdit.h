#pragma once

#include "Config.h"
#include "MapView.h"
#include <json.hpp>
#include <imgui.h>
#include <string_view>
using json = nlohmann::json;

class Map;
struct GLFWwindow;
namespace gl
{
	class Texture;
}


class BrowEdit
{
	const float toolbarHeight = 40.0f;
	float menubarHeight;
	

	gl::Texture* backgroundTexture;
	gl::Texture* iconsTexture;
	std::vector<Map*> maps;
	std::vector<MapView> mapViews;

	struct WindowData
	{
		bool configVisible = false;

		bool openVisible = false;
		std::vector<std::string> openFiles;
		std::size_t openFileSelected = 0;
		std::string openFilter;

	} windowData;

public:
	ImFont* font;
	enum class EditMode
	{
		Texture,
		Object,
		Wall
	} editMode = EditMode::Object;


	GLFWwindow* window;
	double scrollDelta = 0;
	Config config;


	bool glfwBegin();
	void glfwEnd();
	bool glfwLoopBegin();
	void glfwLoopEnd();

	bool imguiBegin();
	void imguiEnd();

	void imguiLoopBegin();
	void imguiLoopEnd();

	void run();

	void loadMap(const std::string& file);
	void showMapWindow(MapView& map);



	void menuBar();
	void toolbar();

	void openWindow();
	void showObjectTree();
	void buildObjectTree(Node* node, Map* map);
	void showObjectProperties();

	std::vector<Node*> selectedNodes;


	bool toolBarToggleButton(const std::string_view &name, int icon, bool* status);
	bool toolBarToggleButton(const std::string_view &name, int icon, bool status);
};