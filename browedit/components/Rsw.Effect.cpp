#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>

#include <iostream>
#include <fstream>
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