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
public:
	TileChangeAction(Gnd::Tile* tile, T* ptr, T startValue, const std::string& action)
	{
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
			gndRenderer->setChunksDirty(); //TODO : only set this specific chunk dirty
			gndRenderer->gndShadowDirty = true;
			for (auto& mv : browEdit->mapViews)
				if (mv.map == map)
					mv.textureGridDirty = true;
		}
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		*ptr = startValue;
		auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
		if (gndRenderer)
		{
			gndRenderer->setChunksDirty(); //TODO : only set this specific chunk dirty
			gndRenderer->gndShadowDirty = true;
			for (auto& mv : browEdit->mapViews)
				if (mv.map == map)
					mv.textureGridDirty = true;
		}
	}
	virtual std::string str()
	{
		if (action == "")
			return "tile " + std::to_string(tile->hash());
		return action + " " + std::to_string(tile->hash());
	};
};