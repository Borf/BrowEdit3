#pragma once

#include "Config.h"
#include "MapView.h"
#include <json.hpp>
#include <imgui.h>
#include <string_view>
class Action;
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

		bool undoVisible = true;


		bool objectWindowVisible = false;

		bool demoWindowVisible = false;

	} windowData;
	std::vector<Action*> undoStack;
	std::vector<Action*> redoStack;


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

	void configBegin();

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
	void saveMap(Map* map);
	void showMapWindow(MapView& map);



	void menuBar();
	void toolbar();

	void openWindow();
	void showObjectTree();
	void buildObjectTree(Node* node, Map* map);
	void showObjectProperties();
	void showUndoWindow();
	void showObjectWindow();

	std::vector<Node*> selectedNodes;

	
	void doAction(Action* action);
	void undo();
	void redo();

	bool toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip);
	bool toolBarToggleButton(const std::string_view &name, int icon, bool status, const char* tooltip);
};