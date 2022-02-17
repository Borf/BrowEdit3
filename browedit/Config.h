#pragma once
#include <glm/glm.hpp>
#include <string>
#include <json.hpp>
#include <vector>
#include <imgui.h>
#include <browedit/util/Util.h>
class BrowEdit;

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

	float toolbarHeight() { return toolbarButtonSize + 8; }

	std::string isValid() const;
	bool showWindow(BrowEdit* browEdit);
	void setupFileIO();
	void setStyle(int style);
public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config,
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
		backup);
};