#pragma once

#include <string>

class Map;
class BrowEdit;

class Action
{
public:
	virtual void perform(Map* map, BrowEdit* browEdit) = 0;
	virtual void undo(Map* map, BrowEdit* browEdit) = 0;
	virtual std::string str() = 0;
};