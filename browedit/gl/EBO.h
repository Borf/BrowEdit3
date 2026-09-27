#pragma once

#include <glad/gl.h>
#include <vector>

namespace gl
{
	class EBO
	{
	private:
		GLuint ebo;
		std::size_t length;

		EBO(const EBO& other)
		{
			throw "do not copy!";
		}

	public:
		EBO()
		{
			length = 0;
			glGenBuffers(1, &ebo);
		}
		~EBO()
		{
			glDeleteBuffers(1, &ebo);
		}

		void setData(const std::vector<unsigned int> &data, GLenum usage)
		{
			this->length = data.size();
			bind();
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * length, data.data(), usage);
		}

		void setData(std::size_t length, unsigned int* data, GLenum usage)
		{
			this->length = length;
			bind();
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * length, data, usage);
		}

		void updateData(std::size_t size, int offset, unsigned int* data)
		{
			bind();
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(unsigned int), size * sizeof(unsigned int), data);
		}

		void bind()
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		}

		void unBind()
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		std::size_t size()
		{
			return length;
		}
	};
}


