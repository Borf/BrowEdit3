#pragma once

#include "Action.h"
#include <vector>
#include <glm/glm.hpp>

class TileSelectAction : public Action
{
	std::vector<glm::ivec2> oldSelection;
	std::vector<glm::ivec2> newSelection;
public:
	TileSelectAction(Map* map, const glm::ivec2& pos, bool keepSelection, bool deSelect);
	TileSelectAction(Map* map, const std::vector<glm::ivec2>& newSelection);

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};

class GatTileSelectAction : public Action
{
	std::vector<glm::ivec2> oldSelection;
	std::vector<glm::ivec2> newSelection;
public:
	GatTileSelectAction(Map* map, const glm::ivec2& pos, bool keepSelection, bool deSelect);
	GatTileSelectAction(Map* map, const std::vector<glm::ivec2>& newSelection);

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};
