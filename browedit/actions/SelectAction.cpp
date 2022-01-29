#include "SelectAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>

SelectAction::SelectAction(Map* map, Node* node, bool keepSelection, bool deSelect)
{
	name = node->name;
	oldSelection = map->selectedNodes;
	if (keepSelection)
		newSelection = oldSelection;

	if (!deSelect)
		newSelection.push_back(node);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [node](Node* n) { return n == node; }));
}

void SelectAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
	}
	map->selectedNodes = newSelection;
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = true;
	}
}

void SelectAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
	}
	map->selectedNodes = oldSelection;
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = true;
	}
}

std::string SelectAction::str()
{
	return "Node selection: " + name;
}
