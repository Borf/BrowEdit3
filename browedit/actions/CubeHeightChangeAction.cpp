#include "CubeHeightChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/GatRenderer.h>
#include <browedit/components/Gat.h>

template<class T, class TC>
CubeHeightChangeAction<T, TC>::CubeHeightChangeAction(const std::map<TC*, float[4]>& oldValues, const std::map<TC*, float[4]>& newValues)
{
	this->oldValues = oldValues;
	this->newValues = newValues;
}

template<class T, class TC>
CubeHeightChangeAction<T, TC>::CubeHeightChangeAction(T* gnd, const std::vector<glm::ivec2>& startSelection)
{
	for (auto t : startSelection)
		for (int i = 0; i < 4; i++)
			oldValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->heights[i];
}

template<class T, class TC>
void CubeHeightChangeAction<T, TC>::setNewHeights(T* gnd, const std::vector<glm::ivec2>& endSelection)
{
	for (auto t : endSelection)
		for (int i = 0; i < 4; i++)
			newValues[gnd->cubes[t.x][t.y]][i] = gnd->cubes[t.x][t.y]->heights[i];
}

template<class T, class TC>
void CubeHeightChangeAction<T, TC>::perform(Map* map, BrowEdit* browEdit)
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
	if (std::is_same<T, Gat>::value)
	{
		auto gatRenderer = map->rootNode->getComponent<GatRenderer>();
		if (gatRenderer)
			gatRenderer->setChunksDirty();
	}
}

template<class T, class TC>
void CubeHeightChangeAction<T, TC>::undo(Map* map, BrowEdit* browEdit)
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
	if (std::is_same<T, Gat>::value)
	{
		auto gatRenderer = map->rootNode->getComponent<GatRenderer>();
		if (gatRenderer)
			gatRenderer->setChunksDirty();
	}
}

template<class T, class TC>
std::string CubeHeightChangeAction<T, TC>::str()
{
	return "Change tile height";
}


template class CubeHeightChangeAction<Gnd, Gnd::Cube>;
template class CubeHeightChangeAction<Gat, Gat::Cube>;
