#pragma once
#include <Windows.h>
#include "Config.h"
#include "MapView.h"
#include <json.hpp>
#include <imgui.h>
#include <string_view>
#include <mutex>
#include <browedit/util/FileIO.h>
class Action;
class Lightmapper;
using json = nlohmann::json;

class Map;
struct GLFWwindow;
namespace gl
{
	class Texture;
}

#define MISSING 63

class BrowEdit
{
	gl::Texture* backgroundTexture;

	gl::Texture* lightTexture;
	gl::Texture* effectTexture;
	gl::Texture* soundTexture;


	std::vector<Map*> maps;
	std::list<MapView> mapViews; //list, because vector reallocates mapviews when pushing back, which breaks activeMapview pointer
public:
	gl::Texture* iconsTexture;
	Lightmapper* lightmapper = nullptr;

	struct WindowData
	{
		bool configVisible = false;

		//bool openVisible = false;
		bool openJustVisible = false;
		std::vector<std::string> openFiles;
		std::size_t openFileSelected = 0;
		std::string openFilter;

		bool undoVisible = true;

		bool helpWindowVisible = false;

		bool objectWindowVisible = true; //TODO: load from config
		util::FileIO::Node* objectWindowSelectedTreeNode = nullptr;

		bool demoWindowVisible = false;

		bool openLightmapSettings = false;

		//progress
		std::mutex progressMutex;
		bool progressWindowVisible = false;
		std::string progressWindowText = "Progress....";
		float progressWindowProgres = .25;
		std::function<void()> progressWindowOnDone;

	} windowData;
	ImFont* font;
	enum class EditMode
	{
		Height,
		Texture,
		Object,
		Wall,
		Gat,
	} editMode = EditMode::Object;

	std::map<std::string, std::vector<std::string>> tagList; // tag -> [ file ], utf8
	std::map<std::string, std::vector<std::string>> tagListReverse; // file -> [ tag ], kr
	GLFWwindow* window;
	double scrollDelta = 0;
	Config config;
	std::vector<std::pair<Node*, glm::vec3>> newNodes;
	MapView* activeMapView = nullptr;
	float statusBarHeight = 10;

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
	void showHelpWindow();
	void showLightmapSettingsWindow();


	bool toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip, ImVec4 tint = ImVec4(1,1,1,1));
	bool toolBarToggleButton(const std::string_view &name, int icon, bool status, const char* tooltip, ImVec4 tint = ImVec4(1, 1, 1, 1));
};