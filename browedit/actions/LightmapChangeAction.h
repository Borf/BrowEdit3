#pragma once

#include "Action.h"
#include <browedit/components/Gnd.h>

class LightmapChangeAction : public Action
{
	std::vector<Gnd::Lightmap*> m_oldLightmaps;
	std::vector<Gnd::Lightmap*> m_newLightmaps;
	std::vector<int> m_oldLightmapIndex;
	std::vector<int> m_newLightmapIndex;
public:
	LightmapChangeAction();
	~LightmapChangeAction();

	void setPreviousData(std::vector<Gnd::Lightmap*> lightmaps, std::vector<Gnd::Tile*> tiles);
	void setCurrentData(std::vector<Gnd::Lightmap*> lightmaps, std::vector<Gnd::Tile*> tiles);

	virtual void perform(Map* map, BrowEdit* browEdit) override;
	virtual void undo(Map* map, BrowEdit* browEdit) override;

	virtual std::string str() override;
};