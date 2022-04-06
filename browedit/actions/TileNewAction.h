#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>

class TileNewAction : public Action
{
	Gnd::Tile* tile;
public:
	TileNewAction(Gnd::Tile* tile);
	~TileNewAction();

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;

	virtual std::string str() override;
};