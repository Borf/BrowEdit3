#pragma once

#include "Action.h"
#include <browedit/components/Rsw.h>
#include <map>

struct Rsw::Water;

class WaterHeightChangeAction : public Action
{
	std::map<Rsw::Water*, float> oldValues;
	std::map<Rsw::Water*, float> newValues;
	std::vector<glm::ivec2> selection;
public:
	WaterHeightChangeAction(const std::map<Rsw::Water*, float>& oldValues, const std::map<Rsw::Water*, float>& newValues, const std::vector<glm::ivec2>& selection);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};