#pragma once

#include "Action.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <map>

class TileChangeAction : public Action
{
	std::map<Gnd::Cube*, float[4]> oldValues;
	std::map<Gnd::Cube*, float[4]> newValues;
public:
	TileChangeAction(const std::map<Gnd::Cube*, float[4]> &oldValues, const std::map<Gnd::Cube*, float[4]> &newValues)
	{
		this->oldValues = oldValues;
		this->newValues = newValues;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		for (auto kv : newValues)
			for(int i = 0; i < 4; i++)
				kv.first->heights[i] = kv.second[i];
		auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
		if(gndRenderer)
			gndRenderer->setChunksDirty();
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		for (auto kv : oldValues)
			for (int i = 0; i < 4; i++)
				kv.first->heights[i] = kv.second[i];
		auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
		if (gndRenderer)
			gndRenderer->setChunksDirty();
	}
	virtual std::string str()
	{
		return "Change tile height";
	};
};