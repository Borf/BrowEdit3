#pragma once

#include "Action.h"
#include <browedit/Node.h>

class NewObjectAction : public Action
{
	Node* node;
public:
	~NewObjectAction();
	NewObjectAction(Node* node);
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};