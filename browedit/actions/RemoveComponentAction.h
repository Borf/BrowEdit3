#pragma once

#include "Action.h"
#include <browedit/Node.h>

template<class T>
class RemoveComponentAction : public Action
{
	std::vector<T*> removedComponents;
	Node* n;
public:
	RemoveComponentAction(Node* n) : n(n)
	{
	}
	~RemoveComponentAction()
	{
		for (auto c : removedComponents)
			delete c;
	}

	virtual void perform(Map* map, BrowEdit* browEdit) override
	{
		removedComponents = n->removeComponent<T>();
	}


	virtual void undo(Map* map, BrowEdit* browEdit) override
	{
		n->addComponents(removedComponents);
		removedComponents.clear();
	}


	virtual std::string str() override
	{
		return "Removing Component from " + n->name;
	}


};