#include "SelectWallAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>

SelectWallAction::SelectWallAction(Map* map, const glm::ivec3 &selection, bool keepSelection, bool deSelect)
{
	if (map == nullptr)
		return;

	name = "Select walls";
	oldSelection = map->wallSelection;
	if (keepSelection)
		newSelection = oldSelection;
	else
		map->wallSelection.clear();

	if (!deSelect)
		newSelection.push_back(selection);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [selection](const glm::ivec3 n) { return n == selection; }));
	map->wallSelection.push_back(selection);
}

void SelectWallAction::perform(Map* map, BrowEdit* browEdit)
{
	map->wallSelection = newSelection;
}

void SelectWallAction::undo(Map* map, BrowEdit* browEdit)
{
	map->wallSelection = oldSelection;
}

std::string SelectWallAction::str()
{
	return "Wall selection";
}
