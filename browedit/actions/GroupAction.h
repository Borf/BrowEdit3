#pragma once

#include "Action.h"
#include <vector>

class GroupAction : public Action
{
	std::vector<Action*> actions;
public:
	~GroupAction()
	{
		for (auto a : actions)
			delete a;
	}
	void addAction(Action* action)
	{
		actions.push_back(action);
	}

	virtual void perform(Map* map, BrowEdit* browEdit) override
	{
		for (auto a : actions)
			a->perform(map, browEdit);
	}
	virtual void undo(Map* map, BrowEdit* browEdit) override
	{
		for (auto a : actions)
			a->undo(map, browEdit);
	}
	virtual std::string str()
	{
		std::string ret;
		for (auto a : actions)
			ret += a->str() + "\n";
		return ret;
	}

};