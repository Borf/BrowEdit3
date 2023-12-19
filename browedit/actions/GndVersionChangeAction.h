#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>
#include <vector>

class GndVersionChangeAction : public Action
{
	int oldValue;
	int newValue;
public:
	GndVersionChangeAction(int oldValue, int newValue);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();
};