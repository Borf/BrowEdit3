#include "Node.h"
#include "components/Component.h"
#include "components/Transform.h"
#include "components/Renderer.h"


Node::Node(const std::string& name, Node* parent) : name(name), parent(parent)
{
	addComponent(new Transform());
	if(parent)
		parent->children.push_back(this);
}

void Node::addComponent(Component* component)
{
	component->node = this;
	components.push_back(component);


	Transform* transform = dynamic_cast<Transform*>(component);
	if (transform)
		this->transform = transform;


}

void Node::traverse(const std::function<void(Node*)>& callBack)
{
	callBack(this);
	for (auto n : children)
		n->traverse(callBack);
}
