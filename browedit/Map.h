#pragma once

#include <string>
#include <vector>
#include <browedit/gl/Shader.h>
class Node;
class Action;
class Rsw;
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

	std::vector<glm::ivec2> getSelectionAroundTiles();


	Node* findAndBuildNode(const std::string &path, Node* root = nullptr);

	Map(const std::string& name, BrowEdit* browEdit);
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
	void setSelectedItemsToFloorHeight(BrowEdit* browEdit);

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
};