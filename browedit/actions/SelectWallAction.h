#pragma once

#include "Action.h"
#include <vector>
#include <glm/glm.hpp>

class Map;
class BrowEdit;

class SelectWallAction : public Action
{
	std::vector<glm::ivec3> oldSelection;
	std::vector<glm::ivec3> newSelection;
	std::string name;
public:
	SelectWallAction(Map* map, const glm::ivec3 &selection, bool keepSelection, bool deSelect);

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;
	virtual std::string str() override;
};