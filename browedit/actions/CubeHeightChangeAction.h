#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>
#include <map>

class CubeHeightChangeAction : public Action
{
	std::map<Gnd::Cube*, float[4]> oldValues;
	std::map<Gnd::Cube*, float[4]> newValues;
public:
	CubeHeightChangeAction(const std::map<Gnd::Cube*, float[4]>& oldValues, const std::map<Gnd::Cube*, float[4]>& newValues);
	CubeHeightChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection);
	void setNewHeights(Gnd* gnd, const std::vector<glm::ivec2>& endSelection);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};