#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>
#include <algorithm>
#include <imGuIZMOquat.h>


RswObject::RswObject(RswObject* other) : position(other->position), rotation(other->rotation), scale(other->scale)
{
}

void RswObject::load(std::istream* is, int version, int buildNumber, bool loadModel)
{
	int type;
	is->read(reinterpret_cast<char*>(&type), sizeof(int));

	if (type == 1)
	{
//		std::cout << "Loading model" << std::endl;
		node->addComponent(new RswModel());
		node->addComponent(new RswModelCollider());
		node->getComponent<RswModel>()->load(is, version, buildNumber, loadModel);
	}
	else if (type == 2)
	{
//		std::cout << "Loading light" << std::endl;
		node->addComponent(new RswLight());
		node->getComponent<RswLight>()->load(is);
		node->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
		node->addComponent(new CubeCollider(5));
	}
	else if (type == 3)
	{
//		std::cout << "Loading sound" << std::endl;
		node->addComponent(new RswSound());
		node->getComponent<RswSound>()->load(is, version);
		node->addComponent(new BillboardRenderer("data\\sound.png", "data\\sound_selected.png"));
		node->addComponent(new CubeCollider(5));
	}
	else if (type == 4)
	{
//		std::cout << "Loading effect" << std::endl;
		node->addComponent(new RswEffect());
		node->getComponent<RswEffect>()->load(is);
		node->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
		node->addComponent(new CubeCollider(5));
	}
	else
		std::cerr << "RSW: Error loading object in RSW, objectType=" << type << std::endl;


}





void RswObject::save(std::ofstream& file, Rsw* rsw)
{
	RswModel* rswModel = nullptr;
	RswEffect* rswEffect = nullptr;
	RswLight* rswLight = nullptr;
	RswSound* rswSound = nullptr;
	int type = 0;
	if (rswModel = node->getComponent<RswModel>())
		type = 1;
	if (rswLight = node->getComponent<RswLight>())
		type = 2;
	if (rswSound = node->getComponent<RswSound>())
		type = 3;
	if (rswEffect = node->getComponent<RswEffect>())
		type = 4;
	file.write(reinterpret_cast<char*>(&type), sizeof(int));
	if (rswModel)		rswModel->save(file, rsw->version, rsw->buildNumber); //meh don't like this if...maybe make this an interface savable
	if (rswEffect)		rswEffect->save(file);
	if (rswLight)		rswLight->save(file);
	if (rswSound)		rswSound->save(file, rsw->version);
}



void RswObject::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<RswObject*> rswObjects;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RswObject>(); }) | std::ranges::views::filter([](RswObject* r) { return r != nullptr; }), std::back_inserter(rswObjects));
	if (rswObjects.size() == 0)
		return;
	ImGui::Text("Object");

	ImGui::PushID("Object");
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
	{
		json clipboard;
		to_json(clipboard, *rswObjects[0]);
		ImGui::SetClipboardText(clipboard.dump(1).c_str());
	}
	if (ImGui::BeginPopupContextItem("CopyPaste"))
	{
		try {
			if (ImGui::MenuItem("Copy"))
			{
				json clipboard;
				to_json(clipboard, *rswObjects[0]);
				ImGui::SetClipboardText(clipboard.dump(1).c_str());
			}
			if (ImGui::MenuItem("Paste (no undo)"))
			{
				auto cb = ImGui::GetClipboardText();
				if (cb)
					for(auto& ptr : rswObjects)
						from_json(json::parse(std::string(cb)), *ptr);
				for (auto renderer : nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RsmRenderer>(); }) | std::ranges::views::filter([](RsmRenderer* r) { return r != nullptr; }))
					renderer->setDirty();
			}
		}
		catch (...) {}
		ImGui::EndPopup();
	}
	ImGui::PopID();


	static bool scaleTogether = false;

	if (util::DragFloat3Multi<RswObject>(browEdit, browEdit->activeMapView->map, rswObjects, "Position", [](RswObject* o) { return &o->position; }, 1.0f, 0.0f, 0.0f))
		for (auto renderer : nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RsmRenderer>(); }) | std::ranges::views::filter([](RsmRenderer* r) { return r != nullptr; }))
			renderer->setDirty();
	if (util::DragFloat3Multi<RswObject>(browEdit, browEdit->activeMapView->map, rswObjects, "Scale", [](RswObject* o) { return &o->scale; }, .1f, 0.0f, 1000.0f, scaleTogether))
		for (auto renderer : nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RsmRenderer>(); }) | std::ranges::views::filter([](RsmRenderer* r) { return r != nullptr; }))
			renderer->setDirty();
	ImGui::SameLine();
	ImGui::Checkbox("##together", &scaleTogether);
	if (util::DragFloat3Multi<RswObject>(browEdit, browEdit->activeMapView->map, rswObjects, "Rotation", [](RswObject* o) { return &o->rotation; }, 1.0f, 0, 360.0f))
		for (auto renderer : nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RsmRenderer>(); }) | std::ranges::views::filter([](RsmRenderer* r) { return r != nullptr; }))
			renderer->setDirty();


	if (rswObjects.size() > 0)
	{
		auto rswObject = rswObjects[0];
		glm::quat q(glm::radians(rswObject->rotation * glm::vec3(1,1,-1)));
		if (ImGui::gizmo3D("Rotation", q))
		{
			auto angles = glm::degrees(glm::eulerAngles(q));
			rswObjects[0]->rotation = angles * glm::vec3(1, 1, -1);
			if(rswObjects[0]->node->getComponent<RsmRenderer>())
				rswObjects[0]->node->getComponent<RsmRenderer>()->setDirty();
		}
	}

}