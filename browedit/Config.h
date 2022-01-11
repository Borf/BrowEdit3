#pragma once
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
	float cameraMouseSpeed = 0.5f;

	std::string isValid() const;
	bool showWindow(BrowEdit* browEdit);
	void setupFileIO();
public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, ropath, grfs, fov, cameraMouseSpeed)
};