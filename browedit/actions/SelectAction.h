#pragma once

#include "Action.h"
#include <vector>

class Node;
class Map;

class SelectAction : public Action
{
	std::vector<Node*> oldSelection;
	std::vector<Node*> newSelection;
	std::string name;
public:
	SelectAction(Map* map, Node* node, bool keepSelection, bool deSelect);

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};