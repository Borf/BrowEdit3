#pragma once

#include <glad/gl.h>
#include <vector>

namespace gl
{
	template <class T>
	class VBO
	{
	private:
		GLuint vbo;

		T* element;
		std::size_t length;
		VBO(const VBO& other)
		{
			throw "do not copy!";
		}

	public:
		VBO()
		{
			length = 0;
			element = NULL;
			glGenBuffers(1, &vbo);
		}
		~VBO()
		{
			glDeleteBuffers(1, &vbo);
		}

		void setData(const std::vector<T> &data, GLenum usage)
		{
			this->length = data.size();
			bind();
			glBufferData(GL_ARRAY_BUFFER, sizeof(T) * length, data.data(), usage);
		}

		void setData(std::size_t length, T* data, GLenum usage)
		{
			this->length = length;
			bind();
			glBufferData(GL_ARRAY_BUFFER, sizeof(T) * length, data, usage);
		}

		void updateData(std::size_t size, int offset, T* data)
		{
			bind();
			glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(T), size * sizeof(T), data);
		}

		void bind()
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
		}

		void unBind()
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		int elementSize()
		{
			return T::getSize();
		}

		std::size_t size()
		{
			return length;
		}

		T* mapData(GLenum access)
		{
			bind();
			element = (T*)glMapBuffer(GL_ARRAY_BUFFER, access);
			return element;
		}
		void unmapData()
		{
			bind();
			glUnmapBuffer(GL_ARRAY_BUFFER);
			element = NULL;
		}

		T& operator [](int index)
		{
			if (element == NULL)
				throw "Use mapData before accessing";
			return element[index];
		}


		void setPointer()
		{
			T::setPointer(T::getSize());
		}
		void unsetPointer()
		{
			T::unsetPointer();
		}

	};
}


