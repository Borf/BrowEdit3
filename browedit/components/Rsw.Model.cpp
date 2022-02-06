#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>

RswModel::RswModel(RswModel* other) : aabb(other->aabb), animType(other->animType), animSpeed(other->animSpeed), blockType(other->blockType), fileName(other->fileName)
{
}

void RswModel::load(std::istream* is, int version, bool loadModel)
{
	auto rswObject = node->getComponent<RswObject>();
	if (version >= 0x103)
	{
		node->name = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 40));

		is->read(reinterpret_cast<char*>(&animType), sizeof(int));
		is->read(reinterpret_cast<char*>(&animSpeed), sizeof(float));
		is->read(reinterpret_cast<char*>(&blockType), sizeof(int));
	}
	std::string fileNameRaw = util::FileIO::readString(is, 80);
	fileName = util::iso_8859_1_to_utf8(fileNameRaw);
	assert(fileNameRaw == util::utf8_to_iso_8859_1(fileName));
	objectName = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 80)); // TODO: Unknown?
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);
	if (loadModel)
	{
		node->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + fileNameRaw));
		node->addComponent(new RsmRenderer());
	}
}


void RswModel::loadExtra(nlohmann::json data)
{
	try {
		givesShadow = data["shadow"];
	}
	catch (...) {}
}



void RswModel::save(std::ofstream& file, int version)
{
	auto rswObject = node->getComponent<RswObject>();
	if (version >= 0x103)
	{
		util::FileIO::writeString(file, util::utf8_to_iso_8859_1(node->name), 40);
		file.write(reinterpret_cast<char*>(&animType), sizeof(int));
		file.write(reinterpret_cast<char*>(&animSpeed), sizeof(float));
		file.write(reinterpret_cast<char*>(&blockType), sizeof(int));
	}
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(fileName), 80);
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(objectName), 80); //unknown
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);
}


nlohmann::json RswModel::saveExtra()
{
	nlohmann::json ret;
	ret["shadow"] = givesShadow;
	return ret;
}




void RswModel::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Model");
	util::DragInt(browEdit, browEdit->activeMapView->map, node, "Animation Type", &animType, 1, 0, 100);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Animation Speed", &animSpeed, 0.01f, 0.0f, 100.0f);
	util::DragInt(browEdit, browEdit->activeMapView->map, node, "Block Type", &blockType, 1, 0, 100);
	util::InputText(browEdit, browEdit->activeMapView->map, node, "Filename", &fileName, ImGuiInputTextFlags_ReadOnly);

	util::Checkbox(browEdit, browEdit->activeMapView->map, node, "Cast Shadow", &givesShadow);

}