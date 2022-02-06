#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>


void RswLight::load(std::istream* is)
{
	auto rswObject = node->getComponent<RswObject>();
	node->name = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 40));
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	rswObject->position *= glm::vec3(1, -1, 1);
	is->read(reinterpret_cast<char*>(todo), sizeof(float) * 10);

	is->read(reinterpret_cast<char*>(glm::value_ptr(color)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(&range), sizeof(float));
}


void RswLight::loadExtra(nlohmann::json data)
{
	try {
		if (data["type"] == "point")
			type = Type::Point;
		if (data["type"] == "spot")
			type = Type::Spot;
		givesShadow = data["shadow"];
		affectShadowMap = data["affectshadow"];
		affectLightmap = data["affectlightmap"];
		cutOff = data["cutoff"];
		intensity = data["intensity"];
	}
	catch (...) {}
}


void RswLight::save(std::ofstream& file)
{
	auto rswObject = node->getComponent<RswObject>();
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(node->name), 40);

	file.write(reinterpret_cast<const char*>(glm::value_ptr(rswObject->position * glm::vec3(1, -1, 1))), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(todo), sizeof(float) * 10);

	file.write(reinterpret_cast<char*>(glm::value_ptr(color)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(&range), sizeof(float));
	//todo: custom light properties
}

nlohmann::json RswLight::saveExtra()
{
	nlohmann::json ret;
	if (type == Type::Point)
		ret["type"] = "point";
	else
		ret["type"] = "spot";
	ret["shadow"] = givesShadow;
	ret["cutoff"] = cutOff;
	ret["intensity"] = intensity;
	ret["affectshadowmap"] = affectShadowMap;
	ret["affectlightmap"] = affectLightmap;

	return ret;
}



float RswLight::realRange()
{
	//formula from http://ogldev.atspace.co.uk/www/tutorial36/tutorial36.html and https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
	float kC = 1;
	float kL = 2.0f / range;
	float kQ = 1.0f / (range * range);
	float maxChannel = glm::max(glm::max(color.r, color.g), color.b);
	float adjustedRange = (-kL + glm::sqrt(kL * kL - 4 * kQ * (kC - 128.0f * maxChannel * intensity))) / (2 * kQ);
	return adjustedRange;
}



void RswLight::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Light");

	if (ImGui::CollapsingHeader("Unknown rubbish"))
	{
		for (int i = 0; i < 10; i++)
		{
			ImGui::PushID(i);
			util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Unknown", &todo[i], 0.01f, -100.0f, 100.0f);
			ImGui::PopID();
		}
	}

	util::ColorEdit3(browEdit, browEdit->activeMapView->map, node, "Color", &color);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Range", &range, 1.0f, 0.0f, 1000.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Intensity", &intensity, 1.00f, 0.0f, 100000.0f);
	util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Cutoff", &cutOff, 0.01f, 0.0f, 1.0f);
	util::Checkbox(browEdit, browEdit->activeMapView->map, node, "Gives Shadows", &givesShadow);
	util::Checkbox(browEdit, browEdit->activeMapView->map, node, "Affects Shadowmap", &affectShadowMap);
	util::Checkbox(browEdit, browEdit->activeMapView->map, node, "Affects Colormap", &affectLightmap);


	ImGui::Separator();
	static std::string templateName = "default";
	ImGui::InputText("Template Name", &templateName);
	if (ImGui::Button("Save as template"))
	{
		json out;
		to_json(out, *this);
		std::ofstream file(("data\\lights\\" + templateName).c_str());
		file << std::setw(2) << out;
	}

}
