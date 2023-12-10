#pragma once

#include "Action.h"
#include <browedit/components/Gat.h>
#include <map>

class GatTileChangeAction : public Action
{
	int oldValue;
	int newValue;
	Gat::Cube* cube;
	glm::ivec2 tile;
public:
	GatTileChangeAction(glm::ivec2 tile, Gat::Cube*, int newValue);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};