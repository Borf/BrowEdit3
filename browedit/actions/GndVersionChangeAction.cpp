#include "GndVersionChangeAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>

GndVersionChangeAction::GndVersionChangeAction(int oldValue, int newValue)
{
	this->oldValue = oldValue;
	this->newValue = newValue;
}

void GndVersionChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd) {
		gnd->version = newValue;
	}
}

void GndVersionChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd) {
		gnd->version = oldValue;
	}
}

std::string GndVersionChangeAction::str()
{
	return "GND version set to " + std::to_string(this->newValue);
}
