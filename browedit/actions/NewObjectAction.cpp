#include "NewObjectAction.h"
#include <browedit/Map.h>

NewObjectAction::NewObjectAction(Node* node) : node(node)
{
}

NewObjectAction::~NewObjectAction()
{
	delete node;
	node = nullptr;
}

void NewObjectAction::perform(Map* map, BrowEdit* browEdit)
{
	node->setParent(map->rootNode);
}

void NewObjectAction::undo(Map* map, BrowEdit* browEdit)
{
	node->setParent(nullptr);
}

std::string NewObjectAction::str()
{
	return "Adding new object";
}
