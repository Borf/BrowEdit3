#pragma once

#include <glad/gl.h>
#include <vector>

namespace gl
{
	template <class T>
	class UBO
	{
	private:
		GLuint ubo;

		T* element;
		UBO(const UBO& other)
		{
			throw "do not copy!";
		}

	public:
		UBO()
		{
			element = NULL;
			glGenBuffers(1, &ubo);

			glBindBuffer(GL_UNIFORM_BUFFER, ubo);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
		}
		~UBO()
		{
			glDeleteBuffers(1, &ubo);
		}

		void setData(T* data)
		{
			bind();
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), data);
		}

		void bind()
		{
			glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		}

		void unBind()
		{
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	};
}
