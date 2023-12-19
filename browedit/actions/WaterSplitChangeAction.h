#pragma once

#include "Action.h"
#include <browedit/components/Rsw.h>
#include <vector>

class WaterSplitChangeAction : public Action
{
	std::vector< std::vector<Rsw::Water>> oldValues;
	std::vector< std::vector<Rsw::Water>> newValues;
	int oldSplitWidth;
	int oldSplitHeight;
	int newSplitWidth;
	int newSplitHeight;
public:
	WaterSplitChangeAction(Rsw::WaterData& waterData, int oldSplitWidth, int oldSplitHeight);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();
};