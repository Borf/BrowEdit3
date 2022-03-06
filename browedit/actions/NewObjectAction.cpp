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
	std::string path = node->name;
	if (path.find("\\") != std::string::npos)
		path = path.substr(0, path.find("\\"));
	else
		path = "";
	node->setParent(map->findAndBuildNode(path));
}

void NewObjectAction::undo(Map* map, BrowEdit* browEdit)
{
	node->setParent(nullptr);
}

std::string NewObjectAction::str()
{
	return "Adding new object";
}
