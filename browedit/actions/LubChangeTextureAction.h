#pragma once

#include "Action.h"
#include <browedit/components/Rsw.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/WaterRenderer.h>

class LubChangeTextureAction : public Action
{
	std::string oldValue;
	std::string newValue;
	LubEffect* lubEffect;
public:
	LubChangeTextureAction(LubEffect *lubEffect, std::string oldValue, std::string newValue)
	{
		this->lubEffect = lubEffect;
		this->oldValue = oldValue;
		this->newValue = newValue;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		this->lubEffect->texture = newValue;
		this->lubEffect->dirty = true;
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		this->lubEffect->texture = oldValue;
		this->lubEffect->dirty = true;
	}
	virtual std::string str()
	{
		return "Texture changed to " + newValue;
	};
};