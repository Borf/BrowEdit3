#pragma once

#include "Action.h"
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>

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
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		*ptr = startValue;
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	virtual std::string str()
	{
		return action + " " + node->name;
	};
};