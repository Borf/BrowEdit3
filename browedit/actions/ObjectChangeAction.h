#pragma once

#include "Action.h"
#include <browedit/Node.h>
#include <browedit/BrowEdit.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/WaterRenderer.h>

template<class T>
class ObjectChangeAction : public Action
{
	T* ptr;
	T startValue;
	T newValue;
	Node* node;
	std::string action;
public:
	ObjectChangeAction(Node* node, T* ptr, T startValue, const std::string &action)
	{
		this->startValue = startValue;
		this->ptr = ptr;
		this->newValue = *ptr;
		this->node = node;
		this->action = action;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		*ptr = newValue;
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->setDirty();
		auto gndRenderer = node->getComponent<GndRenderer>();
		if (gndRenderer)
			gndRenderer->setChunksDirty(); //TODO : only set this specific chunk dirty
		auto waterRenderer = node->getComponent<WaterRenderer>();
		if (waterRenderer) {
			waterRenderer->renderFullWater = false;
			waterRenderer->setDirty();

			for (auto& mv : browEdit->mapViews)
				if (mv.map == map) {
					mv.waterGridDirty = true;
				}
		}
		if ((std::string*)ptr == &(node->name))
			node->onRename(map);
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		*ptr = startValue;
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->setDirty();
		auto gndRenderer = node->getComponent<GndRenderer>();
		if (gndRenderer)
			gndRenderer->setChunksDirty(); //TODO : only set this specific chunk dirty
		auto waterRenderer = node->getComponent<WaterRenderer>();
		if (waterRenderer) {
			waterRenderer->renderFullWater = false;
			waterRenderer->setDirty();

			for (auto& mv : browEdit->mapViews)
				if (mv.map == map) {
					mv.waterGridDirty = true;
				}
		}
		if ((std::string*)ptr == &(node->name))
			node->onRename(map);
	}
	virtual std::string str()
	{
		if (action == "")
			return "";
		return action + " " + node->name;
	};
};