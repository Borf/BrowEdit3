#include <windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>


void RswSound::load(std::istream* is, int version)
{
	auto rswObject = node->getComponent<RswObject>();

	node->name = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 80));

	fileName = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 40));

	is->read(reinterpret_cast<char*>(&unknown7), sizeof(float));
	is->read(reinterpret_cast<char*>(&unknown8), sizeof(float));
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);

	is->read(unknown6, 8);

	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);

	is->read(reinterpret_cast<char*>(&vol), sizeof(float));
	is->read(reinterpret_cast<char*>(&width), sizeof(int));
	is->read(reinterpret_cast<char*>(&height), sizeof(int));
	is->read(reinterpret_cast<char*>(&range), sizeof(float));

	if (version >= 0x0200)
		is->read(reinterpret_cast<char*>(&cycle), sizeof(float));
}



void RswSound::save(std::ofstream& file, int version)
{
	auto rswObject = node->getComponent<RswObject>();

	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(node->name), 40);
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(fileName), 80); //TODO: CHECK IF 80/40 or 40/80

	file.write(reinterpret_cast<char*>(&unknown7), sizeof(float));
	file.write(reinterpret_cast<char*>(&unknown8), sizeof(float));
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);

	file.write(unknown6, 8);

	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);

	file.write(reinterpret_cast<char*>(&vol), sizeof(float));
	file.write(reinterpret_cast<char*>(&width), sizeof(int));
	file.write(reinterpret_cast<char*>(&height), sizeof(int));
	file.write(reinterpret_cast<char*>(&range), sizeof(float));

	if (version >= 0x0200)
		file.write(reinterpret_cast<char*>(&cycle), sizeof(float));
}




void RswSound::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Sound");
	util::InputText(browEdit, browEdit->activeMapView->map, node, "Filename", &fileName);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Volume", &vol, 0.01f, 0.0f, 100.0f);

	util::DragInt(browEdit, browEdit->activeMapView->map, node, "Width", (int*)&width, 1, 0, 10000); //TODO: remove cast
	util::DragInt(browEdit, browEdit->activeMapView->map, node, "Height", (int*)&height, 1, 0, 10000); //TODO: remove cast
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Range", &range, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Cycle", &cycle, 0.01f, 0.0f, 100.0f);
	//no undo for this
	ImGui::InputText("unknown6", unknown6, 8, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Unknown7", &unknown7, 0.01f, 0.0f, 100.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Unknown8", &unknown8, 0.01f, 0.0f, 100.0f);
}