#pragma once

#include <GLFW/glfw3.h>
#include <string>

namespace gl
{
	class Texture
	{
	public:
		GLuint id;
		std::string fileName;
		int width, height;

		Texture(const std::string& fileName, bool flip = false);
		Texture(int width, int height);
		void bind();
		void setSubImage(char* data, int x, int y, int width, int height);

	};
}