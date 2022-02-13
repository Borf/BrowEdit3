#pragma once

#include <string>
#include <iostream>
#include <browedit/util/FileIO.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>

class Image
{
	unsigned char* data;
	int width;
	int height;
public:
	Image(const std::string& fileName);
	float get(const glm::vec2& uv);
};