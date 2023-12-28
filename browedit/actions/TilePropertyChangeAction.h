#pragma once

#pragma once

#include "Action.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/MapView.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>

#include <string>

template<class T>
class TileChangeAction : public Action
{
	T* ptr;
	T startValue;
	T newValue;
	Gnd::Tile* tile;
	std::string action;
	glm::ivec2 position;
public:
	TileChangeAction(glm::ivec2 position, Gnd::Tile* tile, T* ptr, T startValue, const std::string& action)
	{
		this->position = position;
		this->startValue = startValue;
		this->ptr = ptr;
		this->newValue = *ptr;
		this->tile = tile;
		this->action = action;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		*ptr = newValue;
		auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
		if (gndRenderer)
		{
			gndRenderer->setChunkDirty(position.x, position.y);
			gndRenderer->setChunkDirty(position.x - 1, position.y);
			gndRenderer->setChunkDirty(position.x, position.y - 1);
			gndRenderer->setChunkDirty(position.x - 1, position.y - 1);
		}
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		*ptr = startValue;
		auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
		if (gndRenderer)
		{
			gndRenderer->setChunkDirty(position.x, position.y);
			gndRenderer->setChunkDirty(position.x - 1, position.y);
			gndRenderer->setChunkDirty(position.x, position.y - 1);
			gndRenderer->setChunkDirty(position.x - 1, position.y - 1);
		}
	}
	virtual std::string str()
	{
		if (action == "")
			return "tile " + std::to_string(tile->hash());
		return action + " " + std::to_string(tile->hash());
	};
};