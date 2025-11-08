#pragma once

#include <string>
#include <vector>
#include <browedit/gl/Shader.h>
class Node;
class Action;
class Rsw;
class Gnd;
class BrowEdit;
class GroupAction;
namespace gl { class FBO; }

class Map
{
public:
	std::string name;
	Node* rootNode = nullptr;
	std::vector<Action*> undoStack;
	std::vector<Action*> redoStack;
	std::vector<Node*> selectedNodes;
	std::vector<glm::ivec2> tileSelection;
	std::vector<glm::ivec2> gatSelection;
	std::vector<glm::ivec2> waterSelection;
	std::vector<glm::ivec3> wallSelection;

	struct {
		glm::ivec2 selectionStart;
		int selectionSide;
		bool selecting = false;
		bool panning = false;
	} wallSelect;

	bool changed = false;
	bool mapHasNoGnd = false; // This variable is meant to prevent the console from being filled with the "map has no gnd" error

	std::vector<glm::ivec2> getSelectionAroundTiles();

	Node* findAndBuildNode(const std::string &path, Node* root = nullptr);

	Map(const std::string& name, BrowEdit* browEdit);
	Map(const std::string& name, int width, int height, BrowEdit* browEdit);

	~Map();
	void doAction(Action* action, BrowEdit* browEdit);
	void undo(BrowEdit* browEdit);
	void redo(BrowEdit* browEdit);

	GroupAction* tempGroupAction = nullptr;
	void beginGroupAction(const std::string &title = "");
	void endGroupAction(BrowEdit* browEdit);


	glm::vec3 getSelectionCenter();

	void selectSameModels(BrowEdit* browEdit);
	template<class T = RswObject>
	void selectAll(BrowEdit* browEdit);
	void selectInvert(BrowEdit* browEdit);
	void selectNear(float nearDistance, BrowEdit* browEdit);

	void copySelection();
	void pasteSelection(BrowEdit* browEdit);
	void deleteSelection(BrowEdit* browEdit);
	void flipSelection(int axis, BrowEdit* browEdit);
	void invertScale(int axis, BrowEdit* browEdit);
	void setSelectedItemsToFloorHeight(BrowEdit* browEdit);
	void createPrefab(const std::string &fileName, BrowEdit* browEdit);
	void nudgeSelection(int axis, int sign, BrowEdit* browEdit);
	void rotateSelection(int axis, int sign, BrowEdit* browEdit);
	void randomRotateSelection(int axis, BrowEdit* browEdit);

	void exportShadowMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders);
	void exportLightMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders);
	void exportTileColors(BrowEdit* browEdit, bool exportWalls);

	void importShadowMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders);
	void importLightMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders);
	void importTileColors(BrowEdit* browEdit, bool exportWalls);

	void optimizeGndTiles(BrowEdit* browEdit);
	void recalculateQuadTree(BrowEdit* browEdit);

	void growTileSelection(BrowEdit* browEdit);
	void shrinkTileSelection(BrowEdit* browEdit);

	void wallAddSelected(BrowEdit* browEdit);//TODO: is this really the place?
	void wallRemoveSelected(BrowEdit* browEdit);
	void wallReApplySelected(BrowEdit* browEdit);

	void wallFlipSelectedTextureHorizontal(BrowEdit* browEdit);
	void wallFlipSelectedTextureVertical(BrowEdit* browEdit);

};



class WallCalculation
{
	glm::vec2 uv1;
	glm::vec2 uv2;
	glm::vec2 uv3;
	glm::vec2 uv4;

	glm::vec2 uvStart;
	glm::vec2 xInc;
	glm::vec2 yInc;
	int wallWidth;
	int offset;
	float wallTop = 0;
	bool wallTopAuto = true;
	float wallBottom = 0;
	bool wallBottomAuto = true;
	bool autoStraight = true;
public:
	glm::vec2 g_uv1;
	glm::vec2 g_uv2;
	glm::vec2 g_uv3;
	glm::vec2 g_uv4;

	void prepare(BrowEdit* browEdit);
	void calcUV(const glm::ivec3& position, Gnd* gnd);
};