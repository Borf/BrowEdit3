#pragma once

#include "Action.h"
#include <browedit/components/Rsw.h>

class StrChangeSourceAction : public Action
{
	std::string oldValue;
	std::string newValue;
	StrEffect* strEffect;
public:
	StrChangeSourceAction(StrEffect * strEffect, std::string oldValue, std::string newValue)
	{
		this->strEffect = strEffect;
		this->oldValue = oldValue;
		this->newValue = newValue;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		this->strEffect->str = newValue;
		this->strEffect->dirty = true;
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		this->strEffect->str = oldValue;
		this->strEffect->dirty = true;
	}
	virtual std::string str()
	{
		return "Texture changed to " + newValue;
	};
};