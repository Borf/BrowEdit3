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
		for (auto it = actions.rbegin(); it != actions.rend(); it++)
			(*it)->undo(map, browEdit);
	}
	virtual std::string str()
	{
		static const int groupDisplayLimit = 5;
		if (title != "")
			return title;
		std::string ret;
		for (int i = 0; i < actions.size() && i < groupDisplayLimit; i++) {
			auto s = actions[i]->str();
			if (s != "")
				ret += s + (i == groupDisplayLimit - 1 && actions.size() >= groupDisplayLimit ? "..." : "") + "\n";
		}
		return ret;
	}

};