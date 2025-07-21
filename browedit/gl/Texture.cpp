#include <Windows.h> //for warning
#include <glad/gl.h>
#include "Texture.h"

#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <iostream>
#include <stb/stb_image.h>

namespace gl
{
	Texture::Texture(const std::string& fileName, bool flipSelection) : fileName(fileName), flipSelection(flipSelection)
	{
		ids = nullptr;
		if (fileName.find(".tga") == fileName.length() - 4)
			semiTransparent = true;
		if (fileName.find(".gif") != fileName.length() - 4)
			reload();
	}


	Texture::Texture(int width, int height) : width(width), height(height), fileName("")
	{
		ids = new GLuint[1];
		glGenTextures(1, &ids[0]);
		glBindTexture(GL_TEXTURE_2D, ids[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		loaded = true;
	}


	Texture::~Texture()
	{
		if (ids)
		{
			glDeleteTextures(frameCount, ids);
			delete[] ids;
		}
	}

	void Texture::reload()
	{
		if (fileName == "")
			return;
		tryLoaded = true;
		int comp;
		stbi_set_flip_vertically_on_load(flipSelection);

		std::istream* is = util::FileIO::open(fileName);
		if (!is)
		{
			std::cerr << "Texture: Could not open " << fileName << std::endl;
			ids = nullptr;
			return;
		}
		is->seekg(0, std::ios_base::end);
		std::size_t len = is->tellg();
		if (len <= 0 || len > 100 * 1024 * 1024)
		{
			std::cerr << "Texture: Error opening texture " << fileName << ", file is either empty or too large"<<std::endl;
			delete is;
			return;
		}
		char* buffer = new char[len];
		is->seekg(0, std::ios_base::beg);
		is->read(buffer, len);
		delete is;

		if (fileName.substr(fileName.size() - 4) == ".gif")
		{
			int* delays;
			auto data = stbi_load_gif_from_memory((stbi_uc*)buffer, (int)len, &delays, &width, &height, &frameCount, &comp, 4);
			if (ids == nullptr)
			{
				ids = new GLuint[frameCount];
				glGenTextures(frameCount, ids);
			}
			for(int frame = 0; frame < frameCount; frame++)
			{
				glBindTexture(GL_TEXTURE_2D, ids[frame]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data+(width*height*4)*frame);
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			}
			stbi_image_free(data);
		}
		else
		{
			unsigned char* data = stbi_load_from_memory((stbi_uc*)buffer, (int)len, &width, &height, &comp, 4);
			if (!data)
			{
				std::cerr << "Texture: " << fileName << " could not load; error: " << stbi_failure_reason() << std::endl;
				return;
			}

			for (int x = 0; x < width; x++)
			{
				for (int y = 0; y < height; y++)
				{
					if (data[4 * (x + width * y) + 0] > 247 &&
						data[4 * (x + width * y) + 1] < 8 &&
						data[4 * (x + width * y) + 2] > 247)
					{
						data[4 * (x + width * y) + 0] = 0;
						data[4 * (x + width * y) + 1] = 0;
						data[4 * (x + width * y) + 2] = 0;
						data[4 * (x + width * y) + 3] = 0;
					}
				}
			}

			if (ids == nullptr)
			{
				frameCount = 1;
				ids = new GLuint[1];
				glGenTextures(frameCount, ids);
			}
			glBindTexture(GL_TEXTURE_2D, ids[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
			//std::cout << "Texture: loaded " << fileName << std::endl;;
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}

		loaded = true;

	}

	void Texture::setWrapMode(GLuint mode)
	{
		bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
	}

	void Texture::setSubImage(char* data, int x, int y, int width, int height)
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	void Texture::resize(int width, int height)
	{
		bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	GLuint Texture::getAnimatedTextureId()
	{
		if (!tryLoaded)
			reload();
		if (loaded)
			return ids[(int)(glfwGetTime() * 10) % frameCount];
		else
			return 0;
	}

	void Texture::bind()
	{
		if (!tryLoaded)
			reload();
		if (loaded)
		{
			if (frameCount > 1)
				glBindTexture(GL_TEXTURE_2D, ids[(int)(glfwGetTime() * 10) % frameCount]);
			else
				glBindTexture(GL_TEXTURE_2D, ids[0]);
		}
	}
}