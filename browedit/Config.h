#pragma once
#include <glm/glm.hpp>
#include <string>
#include <json.hpp>
#include <vector>
#include <imgui.h>
#include <iostream>
#include <browedit/Hotkey.h>
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
	glm::vec3 wallEditSelectionColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 wallEditHighlightColor = glm::vec3(1.0f, 0.0f, 0.0f);
	ImVec2 thumbnailSize = ImVec2(200, 200);
	bool closeObjectWindowOnAdd = false;
	float toolbarButtonSize = 32;
	ImVec4 toolbarButtonTint = ImVec4(1, 1, 1, 1);
	bool backup = true;
	bool recalculateQuadtreeOnSave = true;
	ImVec4 toolbarButtonsViewOptions = ImVec4(139 / 255.0f, 233 / 255.0f, 253 / 255.0f, 1.0f); //cyan
	ImVec4 toolbarButtonsHeightEdit = ImVec4(80 / 255.0f, 250 / 255.0f, 123 / 255.0f, 1.0f); //green
	ImVec4 toolbarButtonsObjectEdit = ImVec4(255 / 255.0f, 184 / 255.0f, 108/ 255.0f, 1.0f); //orange
	ImVec4 toolbarButtonsTextureEdit = ImVec4(255 / 255.0f, 121 / 255.0f, 198 / 255.0f, 1.0f); //pink
	ImVec4 toolbarButtonsWallEdit = ImVec4(189 / 255.0f, 147 / 255.0f, 249 / 255.0f, 1.0f); //purple
	ImVec4 toolbarButtonsGatEdit = ImVec4(255 / 255.0f, 85 / 255.0f, 85 / 255.0f, 1.0f); //red

	std::vector<float> translateGridSizes = { 0.25f, 0.5f, 1.0f, 2.0f, 2.5f, 5.0f, 10.0f, 20.0f };
	std::vector<float> rotateGridSizes = { 1,22.5, 45,90,180 };
	std::map<std::string, Hotkey> hotkeys;
	std::map<std::string, std::map<std::string, glm::vec4>> colorPresets;

	std::string grfEditorPath = "";
	std::string ffmpegPath = "";

	std::vector<std::string> recentFiles;
	int defaultEditMode = 0;
	float toolbarHeight() { return toolbarButtonSize + 8; }
	int lightmapperThreadCount = 4;
	int lightmapperRefreshTimer = 2;
	bool additiveShadow = true;
	std::string isValid() const;
	bool showWindow(BrowEdit* browEdit);
	void setupFileIO();
	void setStyle(int style);
	void save();
	void defaultHotkeys();
public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_CHECKED(Config,
		ropath,
		grfs,
		fov,
		cameraMouseSpeed,
		style,
		backgroundColor,
		wallEditSelectionColor,
		wallEditHighlightColor,
		thumbnailSize,
		closeObjectWindowOnAdd,
		toolbarButtonSize,
		toolbarButtonTint,
		backup,
		toolbarButtonsViewOptions,
		toolbarButtonsHeightEdit,
		toolbarButtonsObjectEdit,
		toolbarButtonsTextureEdit,
		toolbarButtonsWallEdit,
		toolbarButtonsGatEdit,
		translateGridSizes,
		rotateGridSizes,
		grfEditorPath,
		recentFiles,
		hotkeys,
		defaultEditMode,
		recalculateQuadtreeOnSave,
		colorPresets,
		ffmpegPath,
		lightmapperThreadCount,
		lightmapperRefreshTimer,
		additiveShadow);
};