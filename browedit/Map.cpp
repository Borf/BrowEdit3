#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/actions/Action.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/DeleteObjectAction.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/BrowEdit.h>
#include <browedit/util/ResourceManager.h>

#include <json.hpp>
using json = nlohmann::json;

#include <set>
#include <iostream>
#include <thread>
#include <mutex>


Map::Map(const std::string& name) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);	
	rsw->load(name);
}



void Map::doAction(Action* action, BrowEdit* browEdit)
{
	if (std::find(undoStack.begin(), undoStack.end(), action) != undoStack.end())
	{
		std::cerr << "Double undo!" << std::endl;
		throw "Oops";
		return;
	}
	action->perform(this, browEdit);
	undoStack.push_back(action);

	for (auto a : redoStack)
		delete a;
	redoStack.clear();
}

void Map::redo(BrowEdit* browEdit)
{
	if (redoStack.size() > 0)
	{
		redoStack.front()->perform(this, browEdit);
		undoStack.push_back(redoStack.front());
		redoStack.erase(redoStack.begin());
	}
}
void Map::undo(BrowEdit* browEdit)
{
	if (undoStack.size() > 0)
	{
		undoStack.back()->undo(this, browEdit);
		redoStack.insert(redoStack.begin(), undoStack.back());
		undoStack.pop_back();
	}
}



glm::vec3 Map::getSelectionCenter()
{
	int count = 0;
	glm::vec3 center;
	for (auto n : selectedNodes)
		if (n->getComponent<RswObject>())
		{
			center += n->getComponent<RswObject>()->position;
			count++;
		}
	center /= count;
	return center;
}


void Map::flipSelection(int axis, BrowEdit* browEdit)
{
	glm::vec3 center = getSelectionCenter();
	auto ga = new GroupAction();
	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->position[axis];
			rswObject->position[axis] = center[axis] - (rswObject->position[axis] - center[axis]);
			ga->addAction(new ObjectChangeAction(n, &rswObject->position[axis], orig, "Moving"));
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);
}

void Map::selectNear(float nearDistance, BrowEdit* browEdit)
{
	auto selection = selectedNodes;
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		auto rswModel = n->getComponent<RswModel>();
		auto rswObject = n->getComponent<RswObject>();
		if (!rswObject)
			return;
		auto distance = 999999999.0f;
		for (auto nn : selection)
			if (glm::distance(nn->getComponent<RswObject>()->position, rswObject->position) < distance)
				distance = glm::distance(nn->getComponent<RswObject>()->position, rswObject->position);
		if (distance < nearDistance)
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}


void Map::selectSameModels(BrowEdit* browEdit)
{
	std::set<std::string> files;
	for (auto n : selectedNodes)
	{
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
			files.insert(rswModel->fileName);
	}
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		if (std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end())
			return;
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel && std::find(files.begin(), files.end(), rswModel->fileName) != files.end())
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}
void Map::selectAll(BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		if (std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end())
			return;
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}
void Map::selectInvert(BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		bool selected = std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end();

		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
		{
			auto sa = new SelectAction(this, n, true, selected);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}


void Map::deleteSelection(BrowEdit* browEdit)
{
	if (selectedNodes.size() > 0)
	{
		auto ga = new GroupAction();
		for (auto n : selectedNodes)
			ga->addAction(new DeleteObjectAction(n));
		doAction(ga, browEdit);
	}
}

void Map::copySelection()
{
	json clipboard;
	for (auto n : selectedNodes)
		clipboard.push_back(*n);
	ImGui::SetClipboardText(clipboard.dump(1).c_str());
}


void Map::pasteSelection(BrowEdit* browEdit)
{
	try
	{
		json clipboard = json::parse(ImGui::GetClipboardText());
		if (clipboard.size() > 0)
		{
			for (auto n : browEdit->newNodes) //TODO: should I do this?
				delete n.first;
			browEdit->newNodes.clear();

			for (auto n : clipboard)
			{
				auto newNode = new Node(n["name"].get<std::string>());
				for (auto c : n["components"])
				{
					if (c["type"] == "rswobject")
					{
						auto rswObject = new RswObject();
						from_json(c, *rswObject);
						newNode->addComponent(rswObject);
					}
					if (c["type"] == "rswmodel")
					{
						auto rswModel = new RswModel();
						from_json(c, *rswModel);
						newNode->addComponent(rswModel);
						newNode->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + util::utf8_to_iso_8859_1(rswModel->fileName)));
						newNode->addComponent(new RsmRenderer());
						newNode->addComponent(new RswModelCollider());
					}
					if (c["type"] == "rswlight")
					{
						auto rswLight = new RswLight();
						from_json(c, *rswLight);
						newNode->addComponent(rswLight);
						newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
						newNode->addComponent(new CubeCollider(5));
					}
				}
				browEdit->newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, newNode->getComponent<RswObject>()->position));
			}
			glm::vec3 center(0, 0, 0);
			for (auto& n : browEdit->newNodes)
				center += n.second;
			center /= browEdit->newNodes.size();

			for (auto& n : browEdit->newNodes)
				n.second = n.second - center;
		}
	}
	catch (...) {
		std::cerr << "!!!!!!!!!!! Some sort of error occurred???" << std::endl;
	};
}
