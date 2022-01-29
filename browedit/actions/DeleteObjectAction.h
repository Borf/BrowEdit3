#pragma once

#include "Action.h"
class Node;
class DeleteObjectAction : public Action
{
	Node* node;
	Node* parent;
public:
	DeleteObjectAction(Node* node);
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};