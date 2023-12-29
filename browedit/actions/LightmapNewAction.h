#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>

class LightmapNewAction : public Action
{
	Gnd::Lightmap* lightmap;
public:
	LightmapNewAction(Gnd::Lightmap* lightmap);
	~LightmapNewAction();

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;

	virtual std::string str() override;
};