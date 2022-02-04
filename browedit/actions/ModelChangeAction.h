#pragma once

#include "Action.h"
class Node;

class ModelChangeAction : public Action
{
	Node* node;
	std::string newFileName;
	std::string oldFileName;
public:
	ModelChangeAction(Node* node, const std::string& newFileName); // ISO format
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};