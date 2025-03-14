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
		GLuint* ids = nullptr;
	public:
		inline GLuint id() {
			if (loaded) { return ids[0]; }
			else { return 0; }
		}
		int frameCount = 1;
		std::string fileName;
		int width, height;
		bool tryLoaded = false;
		bool loaded = false;
		bool flipSelection;
		bool semiTransparent = false;

		Texture(int width, int height);
		~Texture();
		void bind();
		void setSubImage(char* data, int x, int y, int width, int height);
		void reload();
		void resize(int width, int height);
		void setWrapMode(GLuint mode);
		GLuint getAnimatedTextureId();

		friend class util::ResourceManager<gl::Texture>;
	};
}