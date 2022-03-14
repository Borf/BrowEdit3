#include <windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <ranges>
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


void RswSound::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<RswSound*> rswSounds;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RswSound>(); }) | std::ranges::views::filter([](RswSound* r) { return r != nullptr; }), std::back_inserter(rswSounds));
	if (rswSounds.size() == 0)
		return;

	if (ImGui::Button("Play Sound"))
	{
		rswSounds[0]->play();
	}

	ImGui::Text("Sound");
	util::InputTextMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Filename", [](RswSound* s) { return &s->fileName; });
	util::DragFloatMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Volume", [](RswSound* s) { return &s->vol; }, 0.01f, 0.0f, 100.0f);

	util::DragIntMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Width", [](RswSound* s) { return (int*)&s->width; }, 1, 0, 10000); //TODO: remove cast
	util::DragIntMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Height", [](RswSound* s) { return (int*)&s->height; }, 1, 0, 10000); //TODO: remove cast
	util::DragFloatMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Range", [](RswSound* s) { return &s->range; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Cycle", [](RswSound* s) { return &s->cycle; }, 0.01f, 0.0f, 100.0f);
	//no undo for this
	//ImGui::InputTextMulti("unknown6", unknown6, 8, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

	util::DragFloatMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Unknown7", [](RswSound* s) { return &s->unknown7; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswSound>(browEdit, browEdit->activeMapView->map, rswSounds, "Unknown8", [](RswSound* s) { return &s->unknown8; }, 0.01f, 0.0f, 100.0f);
}


void RswSound::play()
{
	auto is = util::FileIO::open("data\\wav\\" + fileName);
	if (is != nullptr)
	{
		is->seekg(0, std::ios_base::end);
		std::size_t len = is->tellg();
		char* buffer = new char[len];
		is->seekg(0, std::ios_base::beg);
		is->read(buffer, len);
		delete is;

		PlaySound(buffer, NULL, SND_MEMORY);
		delete[] buffer;
	}
	else
		std::cerr << "Error opening data\\wav\\" << fileName << std::endl;
}