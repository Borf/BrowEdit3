#include "Node.h"
#include "components/Component.h"
#include "components/Renderer.h"
#include "components/Collider.h"
#include "components/LubRenderer.h"
#include "components/Rsm.h"
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/Map.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>


Node::Node(const std::string& name, Node* parent) : name(name), parent(parent)
{
	if (parent)
	{
		parent->children.push_back(this);
		this->root = parent->root;
	}
	root->dirty = true;
}

Node::~Node()
{
	for (auto c : children)
		delete c;
	children.clear();
	for (auto c : components)
		if (dynamic_cast<Rsm*>(c) != nullptr)
			util::ResourceManager<Rsm>::unload(dynamic_cast<Rsm*>(c)); //TODO: remove double cast
		else
			delete c;
}

void Node::addComponent(Component* component)
{
	component->node = this;
	components.push_back(component);
	root->dirty = true;
}

void Node::makeNameUnique(Node* rootNode)
{

	bool exists = false;
	auto& siblings = parent->children;
	for (auto s : siblings)
		if (s->name == name && s != this)
			exists = true;

	if (exists)
	{
		std::string name_ = name;
		try
		{
			if (name.size() > 4 && name[name.size() - 4] == '_' && std::stoi(name.substr(name.size() - 3)) > 0)
				name_ = name_.substr(0, name.size() - 4);
		}
		catch (...) {}

		int index = 0;
		exists = true;
		while (exists)
		{
			index++;
			name = name_ + "_" + std::string(3 - std::min(3, (int)std::to_string(index).length()), '0') + std::to_string(index);
			exists = false;
			for (auto s : siblings)
				if (s->name == name && s != this)
					exists = true;
		}
	}
}


void Node::onRename(Map* map)
{
	if (this->parent != this->root && util::split(name, "\\").size() == 1)
	{
		this->setParent(this->root);
	}
	this->traverse([&](Node* n) //rename all nodes under this one to have the proper name
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
				if(parts.size() > 0)
					parts[parts.size() - 1 - level] = this->name;
				n->name = util::combine(parts, "\\");
			}
		});

	//check if this node has to be moved
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
	//now check for duplicates
	root->traverse([&map](Node* n)
	{
		for (auto i = 0; i < n->children.size(); i++)
		{
			for (auto ii = i + 1; ii < n->children.size();)
			{
				if (n->children[i]->name == n->children[ii]->name)
				{ // we found a duplicate!
					for (auto nn : n->children[ii]->children)
						nn->setParent(n->children[i]);
					//TODO: check if it can safely be removed
					//delete n->children[ii]; //MEMORY LEAK
					n->children.erase(n->children.begin() + ii);
					map->selectedNodes.clear(); //just remove this
				}
				else
					ii++;
			}
		}
	});

}

void Node::addComponentsFromJson(const nlohmann::json& data)
{
	for (auto c : data)
	{
		if (c["type"] == "rswobject")
		{
			auto rswObject = new RswObject();
			from_json(c, *rswObject);
			this->addComponent(rswObject);
		}
		if (c["type"] == "rswmodel")
		{
			auto rswModel = new RswModel();
			from_json(c, *rswModel);
			this->addComponent(rswModel);
			this->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + util::utf8_to_iso_8859_1(rswModel->fileName)));
			this->addComponent(new RsmRenderer());
			this->addComponent(new RswModelCollider());
		}
		if (c["type"] == "rswlight")
		{
			auto rswLight = new RswLight();
			from_json(c, *rswLight);
			this->addComponent(rswLight);
			this->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
			this->addComponent(new CubeCollider(5));
		}
		if (c["type"] == "rsweffect")
		{
			auto rswEffect = new RswEffect();
			from_json(c, *rswEffect);
			this->addComponent(rswEffect);
			this->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
			this->addComponent(new CubeCollider(5));
		}
		if (c["type"] == "lubeffect")
		{
			auto lubEffect = new LubEffect();
			from_json(c, *lubEffect);
			this->addComponent(lubEffect);
			this->addComponent(new LubRenderer());
		}
		if (c["type"] == "rswsound")
		{
			auto rswSound = new RswSound();
			from_json(c, *rswSound);
			this->addComponent(rswSound);
			this->addComponent(new BillboardRenderer("data\\sound.png", "data\\sound_selected.png"));
			this->addComponent(new CubeCollider(5));
		}
	}
}


void Node::setParent(Node* newParent)
{
	root->dirty = true;
	if (parent)
	{
		if(std::find(parent->children.begin(), parent->children.end(), this) != parent->children.end())
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
	root->dirty = true;
}

void Node::removeChild(Node* child)
{
	children.erase(std::remove_if(children.begin(), children.end(), [&child](Node* n) { return n == child; }));
	root->dirty = true;
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




void to_json(nlohmann::json& j, const Node& n) {
	j = nlohmann::json{ {"name", n.name}, {"children", nlohmann::json::array() }, {"components", nlohmann::json::array() } };
	for (auto c : n.components)
		j["components"].push_back(*c);
	for (auto n : n.children)
		j["children"].push_back(*n);
}

void from_json(const nlohmann::json& j, Node& p) {

}