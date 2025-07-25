#include "SelectAction.h"
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>

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

SelectAction::SelectAction(Map* map, std::vector<Node*> nodes, bool keepSelection, bool deSelect)
{
	multiSelection = true;
	name = "none";

	if (nodes.size() > 0) {
		name = "multi selection (" + std::to_string(nodes.size()) + ")";
	}
	oldSelection = map->selectedNodes;
	if (keepSelection)
		newSelection = oldSelection;

	if (!deSelect) {
		for (auto node : nodes)
			newSelection.push_back(node);
	}
	else {
		for (auto node : nodes)
			newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [node](Node* n) { return n == node; }));
	}
}

void SelectAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
		auto billboardRenderer = node->getComponent<BillboardRenderer>();
		if (billboardRenderer)
			billboardRenderer->selected = false;
	}
	map->selectedNodes = newSelection;
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = true;
		auto billboardRenderer = node->getComponent<BillboardRenderer>();
		if (billboardRenderer)
			billboardRenderer->selected = true;
	}
}

void SelectAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
		auto billboardRenderer = node->getComponent<BillboardRenderer>();
		if (billboardRenderer)
			billboardRenderer->selected = false;
	}
	map->selectedNodes = oldSelection;
	for (auto node : map->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = true;
		auto billboardRenderer = node->getComponent<BillboardRenderer>();
		if (billboardRenderer)
			billboardRenderer->selected = true;
	}
}

std::string SelectAction::str()
{
	return "Node selection: " + name;
}
