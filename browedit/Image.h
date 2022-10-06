#pragma once

#include <string>
#include <iostream>
#include <browedit/util/FileIO.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>

class Image
{
public:
	unsigned char* data;
	int width;
	int height;

	bool hasAlpha = false;
	Image(const std::string& fileName);
	//TODO: destructor
	float get(const glm::vec2& uv);
};