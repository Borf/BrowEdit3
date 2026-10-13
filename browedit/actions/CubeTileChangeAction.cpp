#include <Windows.h>
#include "CubeTileChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/WaterRenderer.h>

#define TILEIDX(tile) (tile.y << 16 | tile.x)

CubeTileChangeAction::CubeTileChangeAction(glm::ivec2 tile, Gnd::Cube* cube, int tileUp, int tileFront, int tileSide)
{
	this->selection.push_back(tile);
	this->oldValues[TILEIDX(tile)][0] = cube->tileUp;
	this->oldValues[TILEIDX(tile)][1] = cube->tileFront;
	this->oldValues[TILEIDX(tile)][2] = cube->tileSide;

	this->newValues[TILEIDX(tile)][0] = tileUp;
	this->newValues[TILEIDX(tile)][1] = tileFront;
	this->newValues[TILEIDX(tile)][2] = tileSide;
}

CubeTileChangeAction::CubeTileChangeAction(const std::vector<glm::ivec2>& selection, const std::map<uint32_t, int[3]>& oldValues, const std::map<uint32_t, int[3]>& newValues)
{
	this->selection = selection;
	this->oldValues = oldValues;
	this->newValues = newValues;
}

CubeTileChangeAction::CubeTileChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection)
{
	for (auto t : startSelection)
		for (int i = 0; i < 4; i++)
			oldValues[TILEIDX(t)][i] = gnd->cubes[t.x][t.y]->tileIds[i];
}

void CubeTileChangeAction::setNewTiles(Gnd* gnd, const std::vector<glm::ivec2>& endSelection)
{
	this->selection = endSelection;
	this->shadowDirty = true;
	for (auto t : endSelection)
		for (int i = 0; i < 4; i++)
			newValues[TILEIDX(t)][i] = gnd->cubes[t.x][t.y]->tileIds[i];
}

void CubeTileChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	Gnd* gnd = map->rootNode->getComponent<Gnd>();

	for (auto kv : newValues)
		for (int i = 0; i < 3; i++)
			gnd->cubes[kv.first & 0xFFFF][kv.first >> 16]->tileIds[i] = kv.second[i];
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
	Gnd* gnd = map->rootNode->getComponent<Gnd>();

	for (auto kv : oldValues)
		for (int i = 0; i < 3; i++)
			gnd->cubes[kv.first & 0xFFFF][kv.first >> 16]->tileIds[i] = kv.second[i];
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
