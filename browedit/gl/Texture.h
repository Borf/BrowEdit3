#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <browedit/util/ResourceManager.h>

namespace gl
{
	class Texture
	{
	protected:
		Texture();
		Texture(const std::string& fileName, bool flipSelection = false, bool powerOfTwo = false);
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

		bool powerOfTwo = false;
		bool textureArray = false;
		int potWidth = -1;
		int potHeight = -1;

		static inline bool defaultEnableMipmap = true;

		Texture(int width, int height);
		~Texture();
		void bind();
		void setSubImage(char* data, int x, int y, int width, int height);
		void reload();
		void resize(int width, int height);
		void setWrapMode(GLuint mode);
		void setMipmap(bool value);
		GLuint getAnimatedTextureId();

		friend class util::ResourceManager<gl::Texture>;
	};

	// Power-of-two textures set the dimensions to the nearest multiple of two. For example:
	// A texture of 70x70 would be changed into a 128x128 texture.
	// A texture of 64x128 would be changed into a 64x128 texture.
	// It uses its own class to prevent mixing textures with the ResourceManager (whether they should all be power-of-two is another story).
	class TexturePoT : public Texture
	{
	protected:
		TexturePoT(const std::string& fileName, bool flipSelection = false);
	public:
		TexturePoT(int width, int height);
		friend class util::ResourceManager<gl::TexturePoT>;
	};

	class TextureArray : public Texture
	{
	public:
		TextureArray(int width, int height, int depth);
		void setSubImage(char* data, int x, int y, int z, int width, int height);
	};
}