#include "Node.h"
#include "components/Component.h"
#include "components/Renderer.h"
#include "components/Collider.h"
#include <browedit/util/Util.h>

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


void Node::onRename()
{
	this->traverse([&](Node* n)
		{
			int level = 0;
			Node* nn = n;
			while (nn != this)
			{
				nn = nn->parent;
				level++;
			}
			if (level != 0)
			{
				std::vector<std::string> parts = util::split(n->name, "\\");
				parts[parts.size() - 1 - level] = this->name;
				n->name = util::combine(parts, "\\");
			}
		});

	std::vector<std::string> parts = util::split(this->name, "\\");
	bool positionOk = true;
	Node* nn = this->parent;
	int i = 2;
	while (positionOk && nn && nn->parent && i - 1 < parts.size())
	{
		if (nn->name != parts[parts.size() - i])
			positionOk = false;
		else
		{
			i++;
			nn = nn->parent;
		}
	}
	if (i - 1 != parts.size())
		positionOk = false;
	if (!positionOk)
	{
		//TODO: sync code with Rsw.cpp
		std::function<Node* (Node*, const std::string&)> buildNode;
		buildNode = [&](Node* root, const std::string& path)
		{
			if (path == "")
				return root;
			std::string firstPart = path;
			std::string secondPart = "";
			if (firstPart.find("\\") != std::string::npos)
			{
				firstPart = firstPart.substr(0, firstPart.find("\\"));
				secondPart = path.substr(path.find("\\") + 1);
			}
			for (auto c : root->children)
				if (c->name == firstPart)
					return buildNode(c, secondPart);
			Node* node = new Node(firstPart, root);
			return buildNode(node, secondPart);
		};
		std::string objPath = this->name;
		if (objPath.find("\\") != std::string::npos)
			objPath = objPath.substr(0, objPath.rfind("\\"));
		else
			objPath = "";
		nn = this->parent;
		this->setParent(buildNode(this->root, objPath));

		while (nn != nn->root)
		{
			if (nn->components.size() == 0 && nn->children.size() == 0)
			{
				Node* p = nn->parent;
				nn->parent->removeChild(nn);
				delete nn;
				nn = p;
			}
			else
				nn = nn->parent;
		}


	}
}


void Node::setParent(Node* newParent)
{
	if (parent)
	{
		parent->children.erase(std::remove_if(parent->children.begin(), parent->children.end(), [this](Node* n) { return n == this; }));
	}
	parent = newParent;
	if (parent)
	{
		parent->children.push_back(this);
		this->root = parent->root;
	}
	else
		this->root = this;
}

void Node::removeChild(Node* child)
{
	children.erase(std::remove_if(children.begin(), children.end(), [&child](Node* n) { return n == child; }));
}

void Node::traverse(const std::function<void(Node*)>& callBack)
{
	callBack(this);
	for (auto n : children)
		n->traverse(callBack);
}


std::vector<std::pair<Node*, std::vector<glm::vec3>>> Node::getCollisions(const math::Ray& ray)
{
	std::vector<std::pair<Node*, std::vector<glm::vec3>>> ret;
	traverse([&](Node* n)
	{
		auto collider = n->getComponent<Collider>();
		if (collider)
		{
			auto collisions = collider->getCollisions(ray);
			if (collisions.size() > 0)
				ret.push_back(std::pair < Node*, std::vector<glm::vec3>>(n, collisions));
		}
	});
	return ret;
}