#include "GatTileChangeAction.h"
#include <browedit/components/GatRenderer.h>
#include <browedit/Map.h>
#include <browedit/Node.h>

GatTileChangeAction::GatTileChangeAction(glm::ivec2 tile, Gat::Cube* cube, int newValue)
{
	this->tile = tile;
	this->cube = cube;
	this->oldValue = cube->gatType;
	this->newValue = newValue;
}

void GatTileChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	auto gatRenderer = map->rootNode->getComponent<GatRenderer>();
	cube->gatType = newValue;
	if(gatRenderer)
		gatRenderer->setChunkDirty(tile.x, tile.y);
}

void GatTileChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	auto gatRenderer = map->rootNode->getComponent<GatRenderer>();
	cube->gatType = oldValue;
	if (gatRenderer)
		gatRenderer->setChunkDirty(tile.x, tile.y);
}

std::string GatTileChangeAction::str()
{
	return "Gat tile change";
}
