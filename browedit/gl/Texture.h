#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <browedit/util/ResourceManager.h>

namespace gl
{
	class Texture
	{
	private:
		Texture(const std::string& fileName, bool flipSelection = false);
	public:
		GLuint id;
		std::string fileName;
		int width, height;

		Texture(int width, int height);
		void bind();
		void setSubImage(char* data, int x, int y, int width, int height);

		friend class util::ResourceManager<gl::Texture>;
	};
}