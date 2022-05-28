#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>
#include <map>

template<class T, class TC>
class CubeHeightChangeAction : public Action
{
	std::map<TC*, float[4]> oldValues;
	std::map<TC*, float[4]> newValues;
public:
	CubeHeightChangeAction(const std::map<TC*, float[4]>& oldValues, const std::map<TC*, float[4]>& newValues);
	CubeHeightChangeAction(T* gnd, const std::vector<glm::ivec2>& startSelection);
	void setNewHeights(T* gnd, const std::vector<glm::ivec2>& endSelection);

	virtual void perform(Map* map, BrowEdit* browEdit);
	virtual void undo(Map* map, BrowEdit* browEdit);
	virtual std::string str();;
};