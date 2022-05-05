#include "CubeHeightChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/GndRenderer.h>


CubeHeightChangeAction::CubeHeightChangeAction(const std::map<Gnd::Cube*, float[4]>& oldValues, const std::map<Gnd::Cube*, float[4]>& newValues)
{
	this->oldValues = oldValues;
	this->newValues = newValues;
}

CubeHeightChangeAction::CubeHeightChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection)
{
	for (auto t : startSelection)
		for (int i = 0; i < 4; i++)
			oldValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->heights[i];
}

void CubeHeightChangeAction::setNewHeights(Gnd* gnd, const std::vector<glm::ivec2>& endSelection)
{
	for (auto t : endSelection)
		for (int i = 0; i < 4; i++)
			newValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->heights[i];
}

void CubeHeightChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto kv : newValues)
		for (int i = 0; i < 4; i++)
			kv.first->heights[i] = kv.second[i];
	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd)
		gnd->recalculateNormals();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	if (gndRenderer)
		gndRenderer->setChunksDirty();
}

void CubeHeightChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto kv : oldValues)
		for (int i = 0; i < 4; i++)
			kv.first->heights[i] = kv.second[i];
	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd)
		gnd->recalculateNormals();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	if (gndRenderer)
		gndRenderer->setChunksDirty();
}

std::string CubeHeightChangeAction::str()
{
	return "Change tile height";
}
