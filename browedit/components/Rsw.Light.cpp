#include <Windows.h>
#include "Rsw.h"
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/Map.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>

#include <iostream>
#include <fstream>
#include <json.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <misc/cpp/imgui_stdlib.h>
#include <imGuIZMOquat.h>
#include <ranges>
#include <algorithm>


void RswLight::load(std::istream* is)
{
	auto rswObject = node->getComponent<RswObject>();
	node->name = util::iso_8859_1_to_utf8(util::FileIO::readString(is, 80));

	// Tokei: Read the position of the light from an older file format of BrowEdit.
	// The newer version of BrowEdit will fill these bytes with 0, which will mean that the
	// new position should be used instead.
	glm::vec3 tempPosition;
	is->seekg(-40, std::ios_base::cur);
	is->read(reinterpret_cast<char*>(glm::value_ptr(tempPosition)), sizeof(float) * 3);
	is->seekg(28, std::ios_base::cur);

	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);

	if (glm::length(rswObject->position) == 0) {
		rswObject->position = tempPosition * glm::vec3(1, -1, 1);
	}

	is->read(reinterpret_cast<char*>(glm::value_ptr(color)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(&range), sizeof(float));

	this->cutOff = 0.5f;
	this->intensity = 1.0f;
}


void RswLight::loadExtra(nlohmann::json data)
{
	try {
		if (data["type"] == "point")
			lightType = Type::Point;
		if (data["type"] == "spot")
			lightType = Type::Spot;
		if (data["type"] == "sun")
			lightType = Type::Sun;
		if (data.find("enabled") != data.end())
			enabled = data["enabled"];
		if(data.find("sunMatchRswDirection") != data.end())
			sunMatchRswDirection = data["sunMatchRswDirection"];
		if (data.find("direction") != data.end())
			direction = data["direction"];
		if (data.find("diffuseLighting") != data.end())
			diffuseLighting = data["diffuseLighting"];

		
		givesShadow = data["shadow"];
		affectShadowMap = data["affectshadowmap"];
		affectLightmap = data["affectlightmap"];
		cutOff = data["cutoff"];
		intensity = data["intensity"];
		falloff = data["falloff"].get<std::vector<glm::vec2>>();
		falloffStyle = data["falloffstyle"];
		spotlightWidth = data["spotlightWidth"];

		if (data.find("minShadowDistance") != data.end())
			minShadowDistance = data["minShadowDistance"];
		else
			minShadowDistance = 0;

	}
	catch (...) {}
}


void RswLight::save(std::ofstream& file)
{
	auto rswObject = node->getComponent<RswObject>();
	util::FileIO::writeString(file, util::utf8_to_iso_8859_1(node->name), 80);

	file.write(reinterpret_cast<const char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(glm::value_ptr(color)), sizeof(float) * 3);
	file.write(reinterpret_cast<char*>(&range), sizeof(float));
	//todo: custom light properties
}

nlohmann::json RswLight::saveExtra()
{
	nlohmann::json ret;
	if (lightType == Type::Point)
		ret["type"] = "point";
	else if(lightType == Type::Sun)
		ret["type"] = "sun";
	else
		ret["type"] = "spot";
	ret["enabled"] = enabled;
	ret["shadow"] = givesShadow;
	ret["cutoff"] = cutOff;
	ret["intensity"] = intensity;
	ret["affectshadowmap"] = affectShadowMap;
	ret["affectlightmap"] = affectLightmap;
	ret["falloff"] = falloff;
	ret["falloffstyle"] = falloffStyle;
	ret["minShadowDistance"] = minShadowDistance;
	ret["enabled"] = enabled;
	ret["sunMatchRswDirection"] = sunMatchRswDirection;
	ret["direction"] = direction;
	ret["spotlightWidth"] = spotlightWidth;
	ret["diffuseLighting"] = diffuseLighting;

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
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
	{
		json clipboard;
		to_json(clipboard, *rswLights[0]);
		ImGui::SetClipboardText(clipboard.dump(1).c_str());
	}
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

	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Use diffuse factor", [](RswLight* l) { return &l->diffuseLighting; });

	util::ComboBoxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light type", "Point\0Spot\0Sun\0", [](RswLight* l) { return (int*) &l->lightType; });
	bool differentValues = !std::all_of(rswLights.begin(), rswLights.end(), [&](RswLight* o) { return o->lightType== rswLights.front()->lightType; });
	if (differentValues)
	{
		ImGui::Text("Warning, selection has different types of light");
	}
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Enabled", [](RswLight* l) { return &l->enabled; });

	if (ImGui::Checkbox("Quick preview", &browEdit->activeMapView->enableLightQuickPreview)) {
		if (!browEdit->activeMapView->enableLightQuickPreview) {
			auto gndRenderer = browEdit->activeMapView->map->rootNode->getComponent<GndRenderer>();
			
			if (gndRenderer)
				gndRenderer->quickRenderLightNode = nullptr;
		}
	}

	if (browEdit->activeMapView->enableLightQuickPreview) {
		auto gndRenderer = browEdit->activeMapView->map->rootNode->getComponent<GndRenderer>();

		if (gndRenderer) {
			if (browEdit->activeMapView->enableLightQuickPreview) {
				gndRenderer->quickRenderLightNode = rswLights[0]->node;
				gndRenderer->quickRenderLight_hideOthers = browEdit->activeMapView->hideOtherLightmaps;
			}
		}
	}

	ImGui::SameLine();
	ImGui::Checkbox("Hide other lightmaps", &browEdit->activeMapView->hideOtherLightmaps);

	util::ColorEdit3Multi<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Color", [](RswLight* l) { return &l->color; });
	if(rswLights.front()->lightType != RswLight::Type::Sun)
		util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Range", [](RswLight* l) { return &l->range; }, 1.0f, 0.0f, 1000.0f);
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Gives Shadows", [](RswLight* l) { return &l->givesShadow; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Affects Shadowmap", [](RswLight* l) { return &l->affectShadowMap; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Affects Colormap", [](RswLight* l) { return &l->affectLightmap; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Terrain Shadows", [](RswLight* l) { return &l->shadowTerrain; });
	util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Ignore Terrain Shading", [](RswLight* l) { return &l->noTerrainShading; });
	util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Intensity", [](RswLight* l) { return &l->intensity; }, 0.05f, 0.0f, 100000.0f);

	if (rswLights.front()->lightType == RswLight::Type::Spot)
	{
		util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Spotlight Angle", [](RswLight* l) { return &l->spotlightWidth; }, 0.01f, 0.0f, 1.0f);

		if (util::DragFloat3Multi<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Direction", [](RswLight* l) { return &l->direction; }, 0.05f, -1.0f, 1.0f))
			for (auto& l : rswLights)
				l->direction = glm::normalize(l->direction);


		if (ImGui::gizmo3D("Spotlight Rotation", rswLights.front()->direction, IMGUIZMO_DEF_SIZE, imguiGizmo::modeDirection))
		{
			for (auto& l : rswLights)
				l->direction = glm::normalize(rswLights.front()->direction);
		}

	}
	if (rswLights.front()->lightType != RswLight::Type::Sun)
	{
		util::ComboBoxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Falloff style", "exponential\0spline tweak\0lagrange tweak\0linear tweak\0magic\0s curve\0", [](RswLight* l) { return (int*) & l->falloffStyle; });

		differentValues = !std::all_of(rswLights.begin(), rswLights.end(), [&](RswLight* o) { return o->falloffStyle == rswLights.front()->falloffStyle; });
		if (!differentValues)
		{
			auto falloffStyle = rswLights.front()->falloffStyle;

			util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Minimum Shadow Distance", [](RswLight* l) { return &l->minShadowDistance; }, 1.00f, 0.0f, 100000.0f);
			if (falloffStyle == FalloffStyle::Magic)
			{
				util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Cutoff", [](RswLight* l) { return &l->cutOff; }, 0.01f, 0.0f, 10.0f);
			}
			else if (falloffStyle == FalloffStyle::SplineTweak)
				util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateSpline);
			else if (falloffStyle == FalloffStyle::LagrangeTweak)
				util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateLagrange);
			else if (falloffStyle == FalloffStyle::LinearTweak)
				util::EditableGraphMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Falloff", [](RswLight* l) {return &l->falloff; }, util::interpolateLinear);
			else if (falloffStyle == FalloffStyle::S_Curve)
				util::Graph("Light Falloff", [&](float x) { return util::interpolateSCurve(x) / 2.0f; });
			else if (falloffStyle == FalloffStyle::Exponential)
			{
				bool differentFalloff = !std::all_of(rswLights.begin(), rswLights.end(), [&](RswLight* o) { return o->falloffStyle == rswLights.front()->falloffStyle; });
				util::DragFloatMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Cut off", [](RswLight* l) { return &l->cutOff; }, 0.01f, 0.0f, 10.0f);
				if (differentFalloff)
					util::Graph("Light Falloff", [&](float x) { return 0.0f; });
				else
					util::Graph("Light Falloff", [&](float x) { return 1.0f - glm::pow(x, rswLights.front()->cutOff); });
			}
		}
	}
	else {
		util::CheckboxMulti<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Light Direction follows RSW light direction", [](RswLight* l) { return &l->sunMatchRswDirection; });
		if (!rswLights.front()->sunMatchRswDirection)
		{
			util::DragFloat3Multi<RswLight>(browEdit, browEdit->activeMapView->map, rswLights, "Direction", [](RswLight* l) { return &l->direction; }, 0.05f, -1.0f, 1.0f);

			glm::vec3 dir = rswLights.front()->direction * glm::vec3(-1,-1,-1);

			if (ImGui::gizmo3D("Sunlight Rotation", dir, IMGUIZMO_DEF_SIZE, imguiGizmo::modeDirection))
			{
				for (auto& l : rswLights)
					l->direction = glm::normalize(dir) * glm::vec3(-1, -1, -1);
			}
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
		util::FileIO::reload("data\\lights");
	}
}