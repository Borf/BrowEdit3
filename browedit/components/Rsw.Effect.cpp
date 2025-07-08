#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/MapView.h>
#include <browedit/Node.h>
#include <browedit/Config.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/actions/AddComponentAction.h>
#include <browedit/actions/RemoveComponentAction.h>
#include <browedit/actions/LubChangeTextureAction.h>

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





void RswEffect::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<RswEffect*> rswEffects;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RswEffect>(); }) | std::ranges::views::filter([](RswEffect* r) { return r != nullptr; }), std::back_inserter(rswEffects));
	if (rswEffects.size() == 0)
		return;

	ImGui::Text("Effect");
	browEdit->activeMapView->map->beginGroupAction();
	util::DragIntMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Type", [](RswEffect* e) {return &e->id; }, 1, 0, 500);
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		if (rswEffects[0]->id == 974)
		{
			for (auto n : nodes)
				if (!n->getComponent<LubEffect>())
					browEdit->activeMapView->map->doAction(new AddComponentAction(n, new LubEffect()), browEdit);
		}
		else
		{
			for (auto n : nodes)
				if (n->getComponent<LubEffect>())
					browEdit->activeMapView->map->doAction(new RemoveComponentAction<LubEffect>(n), browEdit);
		}
	}
	browEdit->activeMapView->map->endGroupAction(browEdit);

	int id = rswEffects[0]->id;
	if (previews.find(id) == previews.end())
		previews[id] = util::ResourceManager<gl::Texture>::load("data\\texture\\effect\\" + std::to_string(id) + ".gif");
	ImGui::Image((ImTextureID)(long long)previews[id]->getAnimatedTextureId(), ImVec2(200, 200));

	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Loop", [](RswEffect* e) {return &e->loop; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 1", [](RswEffect* e) {return &e->param1; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 2", [](RswEffect* e) {return &e->param2; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 3", [](RswEffect* e) {return &e->param3; }, 0.01f, 0.0f, 100.0f);
	util::DragFloatMulti<RswEffect>(browEdit, browEdit->activeMapView->map, rswEffects, "Param 4", [](RswEffect* e) {return &e->param4; }, 0.01f, 0.0f, 100.0f);
}



void LubEffect::load(const json& data)
{
	if (data.is_null())
		return;
	from_json(data["dir1"], dir1);
	from_json(data["dir2"], dir2);
	from_json(data["gravity"], gravity);
	from_json(data["pos"], pos);
	from_json(data["radius"], radius);
	if(data.size() == 4)
		from_json(data["color"], color);
	else
	{
		color.r = data["color"][0].get<float>();
		color.g = data["color"][1].get<float>();
		color.b = data["color"][2].get<float>();
		color.a = 255;
	}
	color /= 255.0f;
	from_json(data["rate"], rate);
	from_json(data["size"], size);
	from_json(data["life"], life);
	texture = util::replace(data["texture"], "\\\\", "\\");
	speed = data["speed"][0];
	srcmode = data["srcmode"][0];
	destmode = data["destmode"][0];
	maxcount = data["maxcount"][0];
	zenable = data["zenable"][0];
	
	if (data.find("billboard_off") != data.end())
		billboard_off = data["billboard_off"][0];
	if (data.find("rotate_angle") != data.end())
		from_json(data["rotate_angle"], rotate_angle);
}

void LubEffect::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<LubEffect*> lubEffects;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<LubEffect>(); }) | std::ranges::views::filter([](LubEffect* r) { return r != nullptr; }), std::back_inserter(lubEffects));
	if (lubEffects.size() == 0)
		return;


	ImGui::Text("Lub Effect");
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
	{
		json clipboard;
		to_json(clipboard, *lubEffects[0]);
		ImGui::SetClipboardText(clipboard.dump(1).c_str());
	}
	ImGui::PushID("LubEffect");
	if (ImGui::BeginPopupContextItem("CopyPaste"))
	{
		try {
			if (ImGui::MenuItem("Copy"))
			{
				json clipboard;
				to_json(clipboard, *lubEffects[0]);
				ImGui::SetClipboardText(clipboard.dump(1).c_str());
			}
			if (ImGui::MenuItem("Paste (no undo)"))
			{
				auto cb = ImGui::GetClipboardText();
				if (cb)
					for (auto lubEffect : lubEffects)
					{
						from_json(json::parse(std::string(cb)), *lubEffect);
						//lubEffect->node->getComponent<LubRenderer>()->begin();
					}
			}
		}
		catch (...) {}
		ImGui::EndPopup();
	}
	ImGui::PopID();

	if (browEdit->config.grfEditorPath == "")
		ImGui::Text("Please set up grf editor to edit lub effects, then reload the map");
	else
	{
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "dir1", [](LubEffect* e) {return &e->dir1; }, 0.1f, 0, 0);
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "dir2", [](LubEffect* e) {return &e->dir2; }, 0.1f, 0, 0);
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "gravity", [](LubEffect* e) {return &e->gravity; }, 0.1f, 0, 0);
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "pos", [](LubEffect* e) {return &e->pos; }, 0.1f, 0, 0);
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "radius", [](LubEffect* e) {return &e->radius; }, 0.1f, 0, 0);
		util::ColorEdit4Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "color", [](LubEffect* e) {return &e->color; });
		util::DragFloat2Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "rate", [](LubEffect* e) {return &e->rate; }, 0.1f, 0, 0);
		util::DragFloat2Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "size", [](LubEffect* e) {return &e->size; }, 0.1f, 0, 0);
		util::DragFloat2Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "life", [](LubEffect* e) {return &e->life; }, 0.1f, 0, 0);
		util::InputTextMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "texture", [](LubEffect* e) {return &e->texture;}, [&](Node* node, std::string* ptr, std::string* startValue, const std::string& action) {
			browEdit->activeMapView->map->doAction(new LubChangeTextureAction(node->getComponent<LubEffect>(), *startValue, *ptr), browEdit);
		});
		util::DragFloatMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "speed", [](LubEffect* e) {return &e->speed; }, 0.1f, 0, 0);
		util::DragIntMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "srcmode", [](LubEffect* e) {return &e->srcmode; }, 1, 0, 0);
		util::DragIntMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "destmode", [](LubEffect* e) {return &e->destmode; }, 1, 0, 20);
		util::DragIntMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "maxcount", [](LubEffect* e) {return &e->maxcount; }, 1, 0, 20);
		util::DragIntMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "zenable", [](LubEffect* e) {return &e->zenable; }, 1, 0, 1);
		util::DragIntMulti<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "billboard_off ", [](LubEffect* e) {return &e->billboard_off; }, 1, 0, 1);
		util::DragFloat3Multi<LubEffect>(browEdit, browEdit->activeMapView->map, lubEffects, "rotate_angle", [](LubEffect* e) {return &e->rotate_angle; }, 1.0f, 0, 360);
	}

}