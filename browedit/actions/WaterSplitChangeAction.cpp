#include "WaterSplitChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/WaterRenderer.h>
#include <browedit/components/Rsw.h>

WaterSplitChangeAction::WaterSplitChangeAction(Rsw::WaterData& waterData, int oldSplitWidth, int oldSplitHeight)
{
	this->oldSplitWidth = oldSplitWidth;
	this->oldSplitHeight = oldSplitHeight;
	this->newSplitWidth = waterData.splitWidth;
	this->newSplitHeight = waterData.splitHeight;

	oldValues = waterData.zones;

	newValues.clear();
	newValues.resize(newSplitWidth, std::vector<Rsw::Water>(newSplitHeight));

	for (int y = 0; y < newSplitHeight; y++) {
		for (int x = 0; x < newSplitWidth; x++) {
			if (x < waterData.splitWidth && y < waterData.splitHeight)
				newValues[x][y] = oldValues[x][y];
		}
	}
}

void WaterSplitChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	auto rsw = map->rootNode->getComponent<Rsw>();
	if (rsw) {
		rsw->water.zones = newValues;
		rsw->water.splitHeight = newSplitHeight;
		rsw->water.splitWidth = newSplitWidth;
	}
	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer) {
		waterRenderer->setDirty();
	}
}

void WaterSplitChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	auto rsw = map->rootNode->getComponent<Rsw>();
	if (rsw) {
		rsw->water.zones = oldValues;
		rsw->water.splitHeight = oldSplitHeight;
		rsw->water.splitWidth = oldSplitWidth;
	}
	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer) {
		waterRenderer->setDirty();
	}
}

std::string WaterSplitChangeAction::str()
{
	return "Water split changed";
}
