#include "Node.h"
#include "components/Component.h"
#include "components/Renderer.h"


Node::Node(const std::string& name, Node* parent) : name(name), parent(parent)
{
	if (parent)
	{
		parent->children.push_back(this);
		this->root = parent->root;
	}
}

void Node::addComponent(Component* component)
{
	component->node = this;
	components.push_back(component);

}


void Node::setParent(Node* newParent)
{
	if (parent)
	{
		//TODO: remove from old parent
	}
	parent = newParent;
	parent->children.push_back(this);
	this->root = parent->root;
}

void Node::traverse(const std::function<void(Node*)>& callBack)
{
	callBack(this);
	for (auto n : children)
		n->traverse(callBack);
}
