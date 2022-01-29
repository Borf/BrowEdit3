#include "DeleteObjectAction.h"
#include <browedit/Node.h>

DeleteObjectAction::DeleteObjectAction(Node* node) : node(node)
{
}

void DeleteObjectAction::perform(Map* map, BrowEdit* browEdit)
{
	parent = node->parent;
	node->setParent(nullptr);
}

void DeleteObjectAction::undo(Map* map, BrowEdit* browEdit)
{
	node->setParent(parent);
}

std::string DeleteObjectAction::str()
{
	return "Deleting " + node->name;
}
