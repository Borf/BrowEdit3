#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>
#include <map>

class CubeTileChangeAction : public Action
{
	std::map<Gnd::Cube*, int[3]> oldValues;
	std::map<Gnd::Cube*, int[3]> newValues;
public:
	CubeTileChangeAction(const std::map<Gnd::Cube*, int[3]>& oldValues, const std::map<Gnd::Cube*, int[3]>& newValues);
	CubeTileChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection);
	void setNewTiles(Gnd* gnd, const std::vector<glm::ivec2>& endSelection);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};