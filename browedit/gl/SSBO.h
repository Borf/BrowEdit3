#pragma once

#include <glad/gl.h>
#include <vector>

namespace gl
{
	template <class T>
	class SSBO
	{
	private:
		GLuint ssbo;

		T* element;
		std::size_t length;

		SSBO(const SSBO& other)
		{
			throw "do not copy!";
		}

	public:
		SSBO()
		{
			element = NULL;
			glGenBuffers(1, &ssbo);
		}
		~SSBO()
		{
			glDeleteBuffers(1, &ssbo);
		}

		void setData(const std::vector<T>& data, GLenum usage)
		{
			this->length = data.size();
			bind();
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * length, data.data(), usage);
		}

		void setData(T& data, GLenum usage)
		{
			this->length = 1;
			bind();
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * length, &data, usage);
		}

		void bind()
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		}

		void unBind()
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		GLuint getId()
		{
			return ssbo;
		}

		int elementSize()
		{
			return T::getSize();
		}

		std::size_t size()
		{
			return length;
		}
	};
}
