#pragma once

class Map;
class BrowEdit;

class Action
{
public:
	virtual void perform(Map* map, BrowEdit* browEdit) = 0;
	virtual void undo(Map* map, BrowEdit* browEdit) = 0;
};