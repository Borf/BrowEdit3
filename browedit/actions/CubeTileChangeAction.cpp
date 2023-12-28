#include <Windows.h>
#include "CubeTileChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/WaterRenderer.h>

CubeTileChangeAction::CubeTileChangeAction(glm::ivec2 tile, Gnd::Cube* cube, int tileUp, int tileFront, int tileSide)
{
	this->selection.push_back(tile);
	this->oldValues[cube][0] = cube->tileUp;
	this->oldValues[cube][1] = cube->tileFront;
	this->oldValues[cube][2] = cube->tileSide;

	this->newValues[cube][0] = tileUp;
	this->newValues[cube][1] = tileFront;
	this->newValues[cube][2] = tileSide;
}

CubeTileChangeAction::CubeTileChangeAction(const std::vector<glm::ivec2>& selection, const std::map<Gnd::Cube*, int[3]>& oldValues, const std::map<Gnd::Cube*, int[3]>& newValues)
{
	this->selection = selection;
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
	this->selection = endSelection;
	this->shadowDirty = true;
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
		for (auto t : selection) {
			gndRenderer->setChunkDirty(t.x, t.y);
			gndRenderer->setChunkDirty(t.x - 1, t.y);
			gndRenderer->setChunkDirty(t.x, t.y - 1);
			gndRenderer->setChunkDirty(t.x - 1, t.y - 1);
		}
		if (this->shadowDirty)
			gndRenderer->gndShadowDirty = true;
		for (auto& mv : browEdit->mapViews)
			if (mv.map == map)
				mv.textureGridDirty = true;
	}
	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer)
		waterRenderer->setDirty();
}

void CubeTileChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto kv : oldValues)
		for (int i = 0; i < 3; i++)
			kv.first->tileIds[i] = kv.second[i];
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	if (gndRenderer)
	{
		for (auto t : selection) {
			gndRenderer->setChunkDirty(t.x, t.y);
			gndRenderer->setChunkDirty(t.x - 1, t.y);
			gndRenderer->setChunkDirty(t.x, t.y - 1);
			gndRenderer->setChunkDirty(t.x - 1, t.y - 1);
		}
		if (this->shadowDirty)
			gndRenderer->gndShadowDirty = true;
		for (auto& mv : browEdit->mapViews)
			if (mv.map == map)
				mv.textureGridDirty = true;
	}
	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer)
		waterRenderer->setDirty();
}

std::string CubeTileChangeAction::str()
{
	return "Change tile";
}
