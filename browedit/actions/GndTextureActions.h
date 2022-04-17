#pragma once

#include "Action.h"
#include <map>

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


class GndTextureChangeAction : public Action
{
	std::string oldTexture;
	std::string newTexture;
	int index;
public:
	GndTextureChangeAction(int index, const std::string& fileName);
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;

};