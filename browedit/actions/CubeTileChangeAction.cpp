#include "CubeTileChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/GndRenderer.h>

CubeTileChangeAction::CubeTileChangeAction(Gnd::Cube* cube, int tileUp, int tileFront, int tileSide)
{
	this->oldValues[cube][0] = cube->tileUp;
	this->oldValues[cube][1] = cube->tileFront;
	this->oldValues[cube][2] = cube->tileSide;

	this->newValues[cube][0] = tileUp;
	this->newValues[cube][1] = tileFront;
	this->newValues[cube][2] = tileSide;
}

CubeTileChangeAction::CubeTileChangeAction(const std::map<Gnd::Cube*, int[3]>& oldValues, const std::map<Gnd::Cube*, int[3]>& newValues)
{
	this->oldValues = oldValues;
	this->newValues = newValues;
}

CubeTileChangeAction::CubeTileChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection)
{
	for (auto t : startSelection)
		for (int i = 0; i < 4; i++)
			oldValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->tileIds[i];
}

void CubeTileChangeAction::setNewTiles(Gnd* gnd, const std::vector<glm::ivec2>& endSelection)
{
	for (auto t : endSelection)
		for (int i = 0; i < 4; i++)
			newValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->tileIds[i];
}

void CubeTileChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto kv : newValues)
		for (int i = 0; i < 3; i++)
			kv.first->tileIds[i] = kv.second[i];
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	if (gndRenderer)
	{
		gndRenderer->setChunksDirty(); //TODO: don't double up?
		gndRenderer->gndShadowDirty = true;
	}
}

void CubeTileChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto kv : oldValues)
		for (int i = 0; i < 3; i++)
			kv.first->tileIds[i] = kv.second[i];
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	if (gndRenderer)
	{
		gndRenderer->setChunksDirty();//TODO: don't double up?
		gndRenderer->gndShadowDirty = true;
	}
}

std::string CubeTileChangeAction::str()
{
	return "Change tile";
}
