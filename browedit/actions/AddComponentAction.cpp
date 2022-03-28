#include "AddComponentAction.h"

#include <browedit/Node.h>
#include <browedit/components/Component.h>


AddComponentAction::AddComponentAction(Node* node, Component* newComponent) : node(node), newComponent(newComponent)
{
}

AddComponentAction::~AddComponentAction()
{
	//if the component is not used anymore, delete it
	if(newComponent && 
		(newComponent->node == nullptr || 
		 std::find(newComponent->node->components.begin(), newComponent->node->components.end(), newComponent) == newComponent->node->components.end()))
		delete newComponent;
	newComponent = nullptr;
}

void AddComponentAction::perform(Map* map, BrowEdit* browEdit)
{
	node->addComponent(newComponent);
}

void AddComponentAction::undo(Map* map, BrowEdit* browEdit)
{
	node->removeComponent(newComponent);
}

std::string AddComponentAction::str()
{
	return "Adding component to " + node->name;
}
