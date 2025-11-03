#include <Windows.h>
#include "TileSelectAction.h"

#include <browedit/Map.h>

TileSelectAction::TileSelectAction(Map* map, const glm::ivec2& pos, bool keepSelection, bool deSelect)
{
	oldSelection = map->tileSelection;
	if (keepSelection)
		newSelection = oldSelection;

	if (!deSelect)
		newSelection.push_back(pos);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&pos](const glm::ivec2& n) { return n == pos; }));
}

TileSelectAction::TileSelectAction(Map* map, const std::vector<glm::ivec2>& newSelection)
{
	oldSelection = map->tileSelection;
	this->newSelection = newSelection;
}


void TileSelectAction::perform(Map* map, BrowEdit* browEdit)
{
	map->tileSelection = newSelection;
}

void TileSelectAction::undo(Map* map, BrowEdit* browEdit)
{
	map->tileSelection = oldSelection;
}

std::string TileSelectAction::str()
{
	return "Tile selection";
}



/////////////////


GatTileSelectAction::GatTileSelectAction(Map* map, const glm::ivec2& pos, bool keepSelection, bool deSelect)
{
	oldSelection = map->tileSelection;
	if (keepSelection)
		newSelection = oldSelection;

	if (!deSelect)
		newSelection.push_back(pos);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&pos](const glm::ivec2& n) { return n == pos; }));
}

GatTileSelectAction::GatTileSelectAction(Map* map, const std::vector<glm::ivec2>& newSelection)
{
	oldSelection = map->tileSelection;
	this->newSelection = newSelection;
}


void GatTileSelectAction::perform(Map* map, BrowEdit* browEdit)
{
	map->gatSelection = newSelection;
}

void GatTileSelectAction::undo(Map* map, BrowEdit* browEdit)
{
	map->gatSelection = oldSelection;
}

std::string GatTileSelectAction::str()
{
	return "Gat selection";
}
