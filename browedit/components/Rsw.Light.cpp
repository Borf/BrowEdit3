#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <ranges>
#include <algorithm>


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
		affectShadowMap = data["affectshadowmap"];
		affectLightmap = data["affectlightmap"];
		cutOff = data["cutoff"];
		intensity = data["intensity"];
		falloff = data["falloff"].get<std::vector<glm::vec2>>();
		falloffStyle = data["falloffstyle"];
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
	ret["falloff"] = falloff;
	ret["falloffstyle"] = falloffStyle;

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



void RswLight::buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>& nodes)
{
	std::vector<RswLight*> rswLights;
	std::ranges::copy(nodes | std::ranges::views::transform([](Node* n) { return n->getComponent<RswLight>(); }) | std::ranges::views::filter([](RswLight* r) { return r != nullptr; }), std::back_inserter(rswLights));
	if (rswLights.size() == 0)
		return;

	ImGui::Text("Light");
	ImGui::PushID("Light");
	if (ImGui::BeginPopupContextItem("CopyPaste"))
	{
		try {
			if (ImGui::MenuItem("Copy"))
			{
				json clipboard;
				to_json(clipboard, *rswLights[0]);
				ImGui::SetClipboardText(clipboard.dump(1).c_str());
			}
			if (ImGui::MenuItem("Paste (no undo)"))
			{
				auto cb = ImGui::GetClipboardText();
				if (cb)
					for (auto& ptr : rswLights)
						from_json(json::parse(std::string(cb)), *ptr);
			}
		}
		catch (...) {}
		ImGui::EndPopup();
	}
	ImGui::PopID();

	util::ColorEdit3Multi<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Color", [](RswLight* l) { return &l->color; });
	util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Range", [](RswLight* l) { return &l->range; }, 1.0f, 0.0f, 1000.0f);
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Gives Shadows", [](RswLight* l) { return &l->givesShadow; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Affects Shadowmap", [](RswLight* l) { return &l->affectShadowMap; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Affects Colormap", [](RswLight* l) { return &l->affectLightmap; });

	util::ComboBoxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Falloff style", "exponential\0spline tweak\0lagrange tweak\0linear tweak\0magic\0", [](RswLight* l) { return &l->falloffStyle; });

	bool differentValues = !std::all_of(rswLights.begin(), rswLights.end(), [&](RswLight* o) { return o->falloffStyle == rswLights.front()->falloffStyle; });
	if (!differentValues)
	{
		auto falloffStyle = rswLights.front()->falloffStyle;

		if (falloffStyle == FalloffStyle::Magic)
		{
			util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Intensity", [](RswLight* l) { return &l->intensity; }, 1.00f, 0.0f, 100000.0f);
			util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Cutoff", [](RswLight* l) { return &l->cutOff; }, 0.01f, 0.0f, 1.0f);
		}
		else if (falloffStyle == FalloffStyle::SplineTweak)
			util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateSpline);
		else if (falloffStyle == FalloffStyle::LagrangeTweak)
			util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateLagrange);
		else if (falloffStyle == FalloffStyle::LinearTweak)
			util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateLinear);
		else if (falloffStyle == FalloffStyle::Exponential)
		{
			bool differentFalloff = !std::all_of(rswLights.begin(), rswLights.end(), [&](RswLight* o) { return o->falloffStyle == rswLights.front()->falloffStyle; });
			util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Cutoff", [](RswLight* l) { return &l->cutOff; }, 0.01f, 0.0f, 1.0f);
			if(differentFalloff)
				util::Graph("Light Falloff", [&](float x) { return 0.0f; });
			else
				util::Graph("Light Falloff", [&](float x) { return 1.0f - glm::pow(x, rswLights.front()->cutOff); });
		}
	}


	ImGui::Separator();
	ImGui::Text("Save as template:");
	static std::string templateName = "default";
	ImGui::InputText("Template Name", &templateName);
	if (ImGui::Button("Save as template"))
	{
		json out;
		to_json(out, *rswLights[0]);
		std::ofstream file(("data\\lights\\" + templateName).c_str());
		file << std::setw(2) << out;
	}
}