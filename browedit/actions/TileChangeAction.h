#pragma once

#include "Action.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <map>

class TileChangeAction : public Action
{
	std::map<Gnd::Cube*, float[4]> oldValues;
	std::map<Gnd::Cube*, float[4]> newValues;
public:
	TileChangeAction(const std::map<Gnd::Cube*, float[4]>& oldValues, const std::map<Gnd::Cube*, float[4]>& newValues);
	TileChangeAction(Gnd* gnd, const std::vector<glm::ivec2>& startSelection);
	void setNewHeights(Gnd* gnd, const std::vector<glm::ivec2>& endSelection);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};