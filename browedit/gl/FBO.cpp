#include "FBO.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <stb/stb_image_write.h>
#include <thread>
#include <sstream>

namespace gl
{

	FBO::FBO(int width, int height, bool depth /*= false*/, int textureCount, bool hasDepthTexture)
	{
		this->depthTexture = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
		this->textureCount = textureCount;
		depthBuffer = 0;
		fboId = 0;
		this->width = width;
		this->height = height;

		glGenFramebuffers(1, &fboId);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);


		for (int i = 0; i < textureCount; i++)
		{
			glGenTextures(1, &texid[i]);
			glBindTexture(GL_TEXTURE_2D, texid[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texid[i], 0);
		}




		if (depth)
		{
			glGenRenderbuffers(1, &depthBuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		}

		if (hasDepthTexture)
		{
			glGenTextures(1, &texid[textureCount]);
			glBindTexture(GL_TEXTURE_2D, texid[textureCount]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

			float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texid[textureCount], 0);
			if (textureCount == 0)
				glDrawBuffer(GL_NONE); // No color buffer is drawn to.

			depthTexture = texid[textureCount];
		}

		unbind();
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	FBO::FBO(int width, int height, bool hasDepthTexture, Type buf1, Type buf2, Type buf3, Type buf4)
	{
		this->depthTexture = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
		this->textureCount = textureCount;
		depthBuffer = 0;
		fboId = 0;
		this->width = width;
		this->height = height;

		glGenFramebuffers(1, &fboId);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);

		Type textures[] = { buf1, buf2, buf3, buf4 };
		textureCount = 0;
		for (int i = 0; i < 4; i++)
		{
			if (textures[i] == None)
				continue;
			types[i] = textures[i];
			if (textures[i] == Type::ShadowCube)
			{//http://ogldev.atspace.co.uk/www/tutorial43/tutorial43.html
				glGenTextures(1, &texid[i]);
				glBindTexture(GL_TEXTURE_CUBE_MAP, texid[i]);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				for (int i = 0; i < 6; i++)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
				textureCount++;
			}
			else
			{
				glGenTextures(1, &texid[i]);
				glBindTexture(GL_TEXTURE_2D, texid[i]);
				if (textures[i] == Color)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
				else if (textures[i] == Normal)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
				else if (textures[i] == Position)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
				else if (textures[i] == Depth)
					glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

				textureCount++;
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texid[i], 0);
			}
		}


		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
#ifdef ANDROID
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
#else
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
#endif
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

		if (hasDepthTexture)
		{
			glGenTextures(1, &texid[textureCount]);
			depthTexture = texid[textureCount];
			glBindTexture(GL_TEXTURE_2D, texid[textureCount]);
			//				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			//TODO: either pick one of these depending on if the hasDepthTexture is for a shadow or depth
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texid[textureCount], 0);
			if (textureCount == 0)
				glDrawBuffer(GL_NONE); // No color buffer is drawn to.
		}
		unbind();
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	FBO::~FBO()
	{

	}

	void FBO::bind()
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		if (depthBuffer > 0)
			glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		static GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		if (textureCount > 0)
			glDrawBuffers(textureCount, buffers);
	}

	void FBO::bind(int index)
	{
		if (oldFBO == -1)
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		if (depthBuffer > 0)
			glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		//if (textureCount == 0)
		glDrawBuffer(GL_COLOR_ATTACHMENT0); // No color buffer is drawn to.
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, texid[0], 0); //TODO: texid[0] should be the right texid
	}

	void FBO::unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, glm::max(0, oldFBO));
		if (depthBuffer > 0)
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		if (oldFBO == 0)
		{
			static GLenum buffers[] = { GL_BACK };
			glDrawBuffers(1, buffers);
		}
		else if (oldFBO != -1)
		{
			static GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);
		}
		oldFBO = -1;
	}

	void FBO::use(int offset)
	{
		for (int i = 0; i < textureCount; i++)
		{
			glActiveTexture(GL_TEXTURE0 + i + offset);
			glBindTexture(types[i] == Type::ShadowCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texid[i]);
		}
		if (depthTexture > 0)
		{
			glActiveTexture(GL_TEXTURE0 + textureCount + offset);
			glBindTexture(GL_TEXTURE_2D, texid[textureCount]);
		}
		if (textureCount > 1 || depthTexture > 0)
			glActiveTexture(GL_TEXTURE0);

	}

	int FBO::getHeight()
	{
		return height;
	}

	int FBO::getWidth()
	{
		return width;
	}

	void FBO::resize(int w, int h)
	{
		width = w;
		height = h;
		unbind();
		for (int i = 0; i < textureCount; i++)
		{
			glBindTexture(GL_TEXTURE_2D, texid[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}

		if (depthBuffer != 0)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
		if (depthTexture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

	}

	void FBO::saveAsFile(const std::string& fileName)
	{
		char* data = new char[getWidth() * getHeight() * 4];
		use();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		const int rowSize = getWidth() * 4;
		char* row = new char[rowSize];
		for (int y = 0; y < getHeight() / 2; y++)
		{
			memcpy(row, data + rowSize * y, rowSize);
			memcpy(data + rowSize * y, data + rowSize * (getHeight() - 1 - y), rowSize);
			memcpy(data + rowSize * (getHeight() - 1 - y), row, rowSize);
		}
		delete[] row;

		if (fileName.substr(fileName.size() - 4) == ".bmp")
			stbi_write_bmp(fileName.c_str(), getWidth(), getHeight(), 4, data);
		else if (fileName.substr(fileName.size() - 4) == ".tga")
			stbi_write_tga(fileName.c_str(), getWidth(), getHeight(), 4, data);
		else if (fileName.substr(fileName.size() - 4) == ".png")
			stbi_write_png(fileName.c_str(), getWidth(), getHeight(), 4, data, 4 * getWidth());
		else if (fileName.substr(fileName.size() - 4) == ".jpg")
			stbi_write_jpg(fileName.c_str(), getWidth(), getHeight(), 4, data, 100);
		delete[] data;
	}
	void FBO::saveAsFileBackground(const std::string& fileName, std::function<void()> callback)
	{
		char* data = new char[getWidth() * getHeight() * 4];
		use();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		const int rowSize = getWidth() * 4;
		char* row = new char[rowSize];
		for (int y = 0; y < getHeight() / 2; y++)
		{
			memcpy(row, data + rowSize * y, rowSize);
			memcpy(data + rowSize * y, data + rowSize * (getHeight() - 1 - y), rowSize);
			memcpy(data + rowSize * (getHeight() - 1 - y), row, rowSize);
		}
		delete[] row;
		std::thread thread([data, fileName, this, callback]()
			{
				if (fileName.substr(fileName.size() - 4) == ".bmp")
					stbi_write_bmp(fileName.c_str(), getWidth(), getHeight(), 4, data);
				else if (fileName.substr(fileName.size() - 4) == ".tga")
					stbi_write_tga(fileName.c_str(), getWidth(), getHeight(), 4, data);
				else if (fileName.substr(fileName.size() - 4) == ".png")
					stbi_write_png(fileName.c_str(), getWidth(), getHeight(), 4, data, 4 * getWidth());
				else if (fileName.substr(fileName.size() - 4) == ".jpg")
					stbi_write_jpg(fileName.c_str(), getWidth(), getHeight(), 4, data, 100);
				delete[] data;
				callback();
			});
		thread.detach();
	}

	char* FBO::saveToMemoryPng(int& len)
	{
		int w = getWidth();
		int h = getHeight();

		char* data = new char[w * h * 4];
		use();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		const int rowSize = w * 4;
		char* row = new char[rowSize];
		for (int y = 0; y < h / 2; y++)
		{
			memcpy(row, data + rowSize * y, rowSize);
			memcpy(data + rowSize * y, data + rowSize * (h - 1 - y), rowSize);
			memcpy(data + rowSize * (h - 1 - y), row, rowSize);
		}
		delete[] row;
		char* out;
		std::pair<int*, char**> p(&len, &out);
		stbi_write_png_to_func([](void* context, void* data, int size)
		{
			auto& p = *(std::pair<int*, char**>*)context;
			char*& out = *((char**)p.second);
			out = new char[size];
			memcpy(out, data, size);
			*p.first = size;
		}, &p, w, h, 4, data, 0);
		delete[] data;
		return out;
	}
	char* FBO::saveToMemoryJpeg(int quality, int& len)
	{
		int w = getWidth();
		int h = getHeight();

		char* data = new char[w * h * 4];
		use();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		const int rowSize = w * 4;
		char* row = new char[rowSize];
		for (int y = 0; y < h / 2; y++)
		{
			memcpy(row, data + rowSize * y, rowSize);
			memcpy(data + rowSize * y, data + rowSize * (h - 1 - y), rowSize);
			memcpy(data + rowSize * (h - 1 - y), row, rowSize);
		}
		delete[] row;
		std::stringstream buf;
		stbi_write_jpg_to_func([](void* context, void* data, int size)
			{
				auto& buf = *((std::stringstream*)context);
				buf.write((char*)data, size);
			}, &buf, w, h, 4, data, quality);
		delete[] data;
		len = (int)buf.str().size();
		return (char*)buf.str().c_str();
	}

	char* FBO::saveToMemoryRaw()
	{
		int w = getWidth();
		int h = getHeight();

		char* data = new char[w * h * 3];
		use();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		const int rowSize = w * 3;
		char* row = new char[rowSize];
		for (int y = 0; y < h / 2; y++)
		{
			memcpy(row, data + rowSize * y, rowSize);
			memcpy(data + rowSize * y, data + rowSize * (h - 1 - y), rowSize);
			memcpy(data + rowSize * (h - 1 - y), row, rowSize);
		}
		delete[] row;
		return data;
	}

}