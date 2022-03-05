#pragma once
#include <glm/glm.hpp>
#include <string>
#include <json.hpp>
#include <vector>
#include <imgui.h>
#include <iostream>
#include <browedit/util/Util.h>
class BrowEdit;

#define NLOHMANN_JSON_FROM_CHECKED(v1) if(nlohmann_json_j.find(#v1) != nlohmann_json_j.end()) { nlohmann_json_j.at(#v1).get_to(nlohmann_json_t.v1); } else { std::cerr<<"Missing attribute "#v1<<" in config"<<std::endl; }

#define NLOHMANN_DEFINE_TYPE_INTRUSIVE_CHECKED(Type, ...)  \
    friend void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__)) } \
    friend void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_CHECKED, __VA_ARGS__)) }


class Config
{
public:
	std::string ropath;
	std::vector<std::string> grfs;
	float fov = 45;
	float cameraMouseSpeed = 1.0f;
	int style = 0;
	glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.15f);
	ImVec2 thumbnailSize = ImVec2(200, 200);
	bool closeObjectWindowOnAdd = false;
	float toolbarButtonSize = 32;
	ImVec4 toolbarButtonTint = ImVec4(1, 1, 1, 1);
	bool backup = true;
	ImVec4 toolbarButtonsViewOptions = ImVec4(139 / 255.0f, 233 / 255.0f, 253 / 255.0f, 1.0f); //cyan
	ImVec4 toolbarButtonsHeightEdit = ImVec4(80 / 255.0f, 250 / 255.0f, 123 / 255.0f, 1.0f); //green
	ImVec4 toolbarButtonsObjectEdit = ImVec4(255 / 255.0f, 184 / 255.0f, 108/ 255.0f, 1.0f); //orange

	float toolbarHeight() { return toolbarButtonSize + 8; }

	std::string isValid() const;
	bool showWindow(BrowEdit* browEdit);
	void setupFileIO();
	void setStyle(int style);
public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_CHECKED(Config,
		ropath,
		grfs,
		fov,
		cameraMouseSpeed,
		style,
		backgroundColor,
		thumbnailSize,
		closeObjectWindowOnAdd,
		toolbarButtonSize,
		toolbarButtonTint,
		backup,
		toolbarButtonsViewOptions,
		toolbarButtonsHeightEdit,
		toolbarButtonsObjectEdit);
};