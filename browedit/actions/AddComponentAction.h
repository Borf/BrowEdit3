#pragma once

#include "Action.h"

class Node;
class Component;

class AddComponentAction : public Action
{
	Node* node;
	Component* newComponent;
public:
	AddComponentAction(Node* node, Component* newComponent);
	~AddComponentAction();
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};