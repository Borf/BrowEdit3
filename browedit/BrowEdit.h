#pragma once
#include <Windows.h>
#include "Config.h"
#include "MapView.h"
#include <json.hpp>
#include <imgui.h>
#include <string_view>
#include <mutex>
#include <browedit/util/FileIO.h>
#include <browedit/components/Gnd.h>
class Action;
class Lightmapper;
using json = nlohmann::json;

class Map;
class Hotkey;
struct GLFWwindow;
enum class HotkeyAction;
namespace gl
{
	class Texture;
}

#define MISSING 63

class CopyCube : public Gnd::Cube
{
public:
	glm::ivec2 pos;
	CopyCube() {}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(CopyCube, h1, h2, h3, h4, tileUp, tileFront, tileSide, pos, normal, normals);
};

class BrowEdit
{
	gl::Texture* backgroundTexture;

	gl::Texture* lightTexture;
	gl::Texture* effectTexture;
	gl::Texture* soundTexture;
	gl::Texture* prefabTexture;

	std::vector<Map*> maps;
public:
	std::list<MapView> mapViews; //list, because vector reallocates mapviews when pushing back, which breaks activeMapview pointer
	gl::Texture* iconsTexture;
	gl::Texture* gatTexture;
	Lightmapper* lightmapper = nullptr;

	struct WindowData
	{
		bool configVisible = false;

		//bool openVisible = false;
		bool openJustVisible = false;
		std::vector<std::string> openFiles;
		std::size_t openFileSelected = 0;
		std::string openFilter;

		bool exportVisible = false;
		Map* exportMap = nullptr;
		class ExportInfo
		{
		public:
			std::string filename;
			std::string type;
			std::string source;
			bool enabled;
			std::vector<ExportInfo*> linkedForward; //textures used by this model
			std::vector<ExportInfo*> linkedBackward; //models used by this texture
		};
		std::list<ExportInfo> exportToExport;
		std::string exportFolder = "export\\";

		bool undoVisible = true;

		bool helpWindowVisible = false;

		bool objectWindowVisible = true; //TODO: load from config
		util::FileIO::Node* objectWindowSelectedTreeNode = nullptr;

		bool demoWindowVisible = false;

		bool hotkeyEditWindowVisible = false;
		std::map<std::string, Hotkey> hotkeys;

		bool openLightmapSettings = false;


		bool openPrefabPopup = false;


		//progress
		std::mutex progressMutex;
		bool progressWindowVisible = false;
		std::string progressWindowText = "Progress....";
		float progressWindowProgres = .25;
		std::function<void()> progressWindowOnDone;
		std::function<void()> progressCancel;

		bool textureManageWindowVisible = true; //TODO: load from config
		util::FileIO::Node* textureManageWindowSelectedTreeNode = nullptr;
		std::map<std::string, gl::Texture*> textureManageWindowCache;
	} windowData;
	ImFont* font;
	enum class EditMode
	{
		Height,
		Texture,
		Object,
		Wall,
		Gat,
	} editMode = EditMode::Gat;
	
	enum class SelectTool
	{
		Rectangle,
		Lasso,
		WandTex,
		WandHeight,
		AllTex,
		AllHeight,
	} selectTool = SelectTool::Rectangle;
	enum TextureBrushMode
	{
		Stamp,
		Select,
		Fill
	} textureBrushMode = TextureBrushMode::Stamp;
	
	bool heightDoodle = true;
	bool gatDoodle = false;

	std::map<std::string, std::vector<std::string>> tagList; // tag -> [ file ], utf8
	std::map<std::string, std::vector<std::string>> tagListReverse; // file -> [ tag ], kr
	GLFWwindow* window;
	double scrollDelta = 0;
	Config config;
	std::vector<std::pair<Node*, glm::vec3>> newNodes;
	glm::vec3 newNodesCenter;
	bool newNodeHeight = false;
	std::vector<CopyCube*> newCubes;
	MapView* activeMapView = nullptr;
	float statusBarHeight = 10;
	Map* textureStampMap = nullptr;
	std::vector<std::vector<Gnd::Tile*>> textureStamp;
	glm::ivec2 textureFillOffset;


	void configBegin();

	bool glfwBegin();
	void glfwEnd();
	bool glfwLoopBegin();
	void glfwLoopEnd();

	bool imguiBegin();
	void imguiEnd();

	void imguiLoopBegin();
	void imguiLoopEnd();
	void registerActions();
	bool hotkeyMenuItem(const std::string& title, HotkeyAction action);

	void run();

	void loadMap(const std::string file);
	void saveMap(Map* map);
	void saveAsMap(Map* map);
	void exportMap(Map* map);
	void showMapWindow(MapView& map, float deltaTime);

	void saveTagList();

	void menuBar();
	void toolbar();

	void showOpenWindow();
	void showExportWindow();
	void openWindow();
	void showObjectTree();
	void buildObjectTree(Node* node, Map* map);
	void showObjectProperties();
	void showUndoWindow();
	void showObjectWindow();
	void showHelpWindow();
	void showLightmapSettingsWindow();
	void showTextureBrushWindow();
	void showTextureManageWindow();
	void showHotkeyEditorWindow();

	void copyTiles();
	void pasteTiles();

	bool toolBarToggleButton(const std::string_view &name, int icon, bool* status, const char* tooltip, ImVec4 tint = ImVec4(1,1,1,1));
	bool toolBarToggleButton(const std::string_view &name, int icon, bool status, const char* tooltip, ImVec4 tint = ImVec4(1, 1, 1, 1));
	bool toolBarButton(const std::string_view& name, int icon, const char* tooltip, ImVec4 tint);

	bool hotkeyInputBox(const char* title, Hotkey& hotkey);
};