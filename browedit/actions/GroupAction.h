#pragma once

#include "Action.h"
#include <vector>
#include <string>

class GroupAction : public Action
{
	std::vector<Action*> actions;
	std::string title;
public:
	GroupAction(const std::string& title = "") : title(title)
	{
	}
	~GroupAction()
	{
		for (auto a : actions)
			delete a;
	}
	void addAction(Action* action)
	{
		actions.push_back(action);
	}
	bool isEmpty()
	{
		return actions.size() == 0;
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
		if (title != "")
			return title;
		std::string ret;
		for (auto a : actions)
		{
			auto s = a->str();
			if(s != "")
				ret += s + "\n";
		}
		return ret;
	}

};