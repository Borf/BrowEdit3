#include "WaterHeightChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/BrowEdit.h>
#include <browedit/components/WaterRenderer.h>

WaterHeightChangeAction::WaterHeightChangeAction(const std::map<Rsw::Water*, float>& oldValues, const std::map<Rsw::Water*, float>& newValues, const std::vector<glm::ivec2>& selection)
{
	this->selection = selection;
	this->oldValues = oldValues;
	this->newValues = newValues;
}

void WaterHeightChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto kv : newValues)
		kv.first->height = kv.second;

	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer)
		waterRenderer->setDirty();
	for (auto& mv : browEdit->mapViews)
		if (mv.map == map) {
			mv.textureGridDirty = true;
			mv.waterGridDirty = true;
		}
}

void WaterHeightChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto kv : oldValues)
		kv.first->height = kv.second;

	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();
	if (waterRenderer)
		waterRenderer->setDirty();
	for (auto& mv : browEdit->mapViews)
		if (mv.map == map) {
			mv.textureGridDirty = true;
			mv.waterGridDirty = true;
		}
}

std::string WaterHeightChangeAction::str()
{
	return "Change water height";
}
