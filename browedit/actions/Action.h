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

	// Required for classes that inherit from Action.
	// Otherwise "delete subActionType;" will not trigger the deconstructor.
	virtual ~Action() {

	}

	virtual void perform(Map* map, BrowEdit* browEdit) = 0;
	virtual void undo(Map* map, BrowEdit* browEdit) = 0;
	virtual std::string str() = 0;
};