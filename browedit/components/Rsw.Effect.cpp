#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>

#include <iostream>
#include <fstream>
#include <ranges>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>



void RswEffect::load(std::istream* is)
{
	auto rswObject = node->getComponent<RswObject>();
	node->name = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 80));
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(&id), sizeof(int));
	is->read(reinterpret_cast<char*>(&loop), sizeof(float));
	is->read(reinterpret_cast<char*>(&param1), sizeof(float));
	is->read(reinterpret_cast<char*>(&param2), sizeof(float));
	is->read(reinterpret_cast<char*>(&param3), sizeof(float));
	is->read(reinterpret_cast<char*>(&param4), sizeof(float));
}

void RswEffect::save(std::ofstream& file)
{
	auto rswObject = node->getComponent<RswObject>();
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(node->name), 80);
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(&id), sizeof(int));
	file.write(reinterpret_cast<char*>(&loop), sizeof(float));
	file.write(reinterpret_cast<char*>(&param1), sizeof(float));
	file.write(reinterpret_cast<char*>(&param2), sizeof(float));
	file.write(reinterpret_cast<char*>(&param3), sizeof(float));
	file.write(reinterpret_cast<char*>(&param4), sizeof(float));
}




void RswEffect::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Effect");
	util::DragInt(browEdit, browEdit->activeMapView->map, node, "Type", &id, 1, 0, 500); //TODO: change this to a combobox
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Loop", &loop, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Param 1", &param1, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Param 2", &param2, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Param 3", &param3, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Param 4", &param4, 0.01f, 0.0f, 100.0f);
}


void RswEffect::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<RswEffect*> rswEffects;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RswEffect>(); }) | std::ranges::views::filter([](RswEffect* r) { return r != nullptr; }), std::back_inserter(rswEffects));
	if (rswEffects.size() == 0)
		return;

	ImGui::Text("Effect");
	util::DragIntMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Type", [](RswEffect* e) {return &e->id; }, 1, 0, 500); //TODO: change this to a combobox
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Loop", [](RswEffect* e) {return &e->loop; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 1", [](RswEffect* e) {return &e->param1; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 2", [](RswEffect* e) {return &e->param2; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 3", [](RswEffect* e) {return &e->param3; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 4", [](RswEffect* e) {return &e->param4; }, 0.01f, 0.0f, 100.0f);

}