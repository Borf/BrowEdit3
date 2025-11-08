#pragma once

#include "Action.h"
#include <map>
#include <vector>

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
	int index;
	std::string fileName;
	std::vector<int> tiles;
public:
	GndTextureDelAction(int index);
	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
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