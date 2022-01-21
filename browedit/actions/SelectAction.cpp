#include "SelectAction.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>

SelectAction::SelectAction(BrowEdit* browEdit, Node* node, bool keepSelection, bool deSelect)
{
	name = node->name;
	oldSelection = browEdit->selectedNodes;
	if (keepSelection)
		newSelection = oldSelection;

	if (!deSelect)
		newSelection.push_back(node);
	else
		newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [node](Node* n) { return n == node; }));
}

void SelectAction::perform(Map* map, BrowEdit* browEdit)
{
	for (auto node : browEdit->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
	}
	browEdit->selectedNodes = newSelection;
	for (auto node : browEdit->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = true;
	}
}

void SelectAction::undo(Map* map, BrowEdit* browEdit)
{
	for (auto node : browEdit->selectedNodes)
	{
		auto rsmRenderer = node->getComponent<RsmRenderer>();
		if (rsmRenderer)
			rsmRenderer->selected = false;
	}
	browEdit->selectedNodes = oldSelection;
	for (auto node : browEdit->selectedNodes)
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
