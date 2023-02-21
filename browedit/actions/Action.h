#pragma once

#include <string>
#include <chrono>
#include <format>

class Map;
class BrowEdit;

class Action
{
public:
	std::string time;
	Action()
	{
		const auto now = std::chrono::system_clock::now();
		time = std::format("{:%H:%M:%OS}", now);
	}

	virtual void perform(Map* map, BrowEdit* browEdit) = 0;
	virtual void undo(Map* map, BrowEdit* browEdit) = 0;
	virtual std::string str() = 0;
};