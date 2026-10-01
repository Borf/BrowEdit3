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
		GLuint bindingIndex;

		UBO(const UBO& other)
		{
			throw "do not copy!";
		}

	public:
		UBO(GLuint bindingIndex = 0)
		{
			element = NULL;
			this->bindingIndex = bindingIndex;
			glGenBuffers(1, &ubo);

			bind();
			glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
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
			glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, ubo);
		}

		void unBind()
		{
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	};
}
