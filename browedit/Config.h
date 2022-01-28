#pragma once
#include <glm/glm.hpp>
#include <string>
#include <json.hpp>
#include <vector>
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

	std::string isValid() const;
	bool showWindow(BrowEdit* browEdit);
	void setupFileIO();
public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, ropath, grfs, fov, cameraMouseSpeed, style, backgroundColor.r, backgroundColor.g, backgroundColor.b)
};