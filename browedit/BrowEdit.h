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
	std::list<MapView> mapViews; //list, because vector reallocates mapviews when pushing back, which breaks activeMapview pointer

	struct WindowData
	{
		bool configVisible = false;

		//bool openVisible = false;
		bool openJustVisible = false;
		std::vector<std::string> openFiles;
		std::size_t openFileSelected = 0;
		std::string openFilter;

		bool undoVisible = true;


		bool objectWindowVisible = true; //TODO: load from config

		bool demoWindowVisible = false;
		bool progressWindowVisible = false;
		std::string progressWindowText = "Progress....";
		float progressWindowProgres = .25;
		std::function<void()> progressWindowOnDone;

	} windowData;
public:
	ImFont* font;
	enum class EditMode
	{
		Texture,
		Object,
		Wall
	} editMode = EditMode::Object;

	std::map<std::string, std::vector<std::string>> tagList; // tag -> [ file ], utf8
	std::map<std::string, std::vector<std::string>> tagListReverse; // file -> [ tag ], kr
	GLFWwindow* window;
	double scrollDelta = 0;
	Config config;
	std::vector<std::pair<Node*, glm::vec3>> newNodes;
	MapView* activeMapView = nullptr;


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

	void saveTagList();

	void menuBar();
	void toolbar();

	void showOpenWindow();
	void openWindow();
	void showObjectTree();
	void buildObjectTree(Node* node, Map* map);
	void showObjectProperties();
	void showUndoWindow();
	void showObjectWindow();


	bool toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip);
	bool toolBarToggleButton(const std::string_view &name, int icon, bool status, const char* tooltip);
};