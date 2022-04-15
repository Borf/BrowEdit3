#pragma once

#include "Action.h"

class GndTextureAddAction : public Action
{
	std::string fileName;
public:
	GndTextureAddAction(const std::string& fileName);
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};

class GndTextureDelAction : public Action
{
public:
};