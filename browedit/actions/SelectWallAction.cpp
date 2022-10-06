#include "SelectWallAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>

extern std::vector<glm::ivec3> selectedWalls; //eww


SelectWallAction::SelectWallAction(Map* map, const glm::ivec3 &selection, bool keepSelection, bool deSelect)
{
	name = "Select walls";
	oldSelection = selectedWalls;
	if (keepSelection)
		newSelection = oldSelection;
	else
		selectedWalls.clear();

	if (!deSelect)
		newSelection.push_back(selection);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [selection](const glm::ivec3 n) { return n == selection; }));
	selectedWalls.push_back(selection);
}

void SelectWallAction::perform(Map* map, BrowEdit* browEdit)
{
	selectedWalls = newSelection;
}

void SelectWallAction::undo(Map* map, BrowEdit* browEdit)
{
	selectedWalls = oldSelection;
}

std::string SelectWallAction::str()
{
	return "Wall selection";
}
